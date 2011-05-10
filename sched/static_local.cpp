#include <iostream>
#include <boost/thread/barrier.hpp>

#include "sched/static_local.hpp"
#include "db/database.hpp"
#include "db/tuple.hpp"
#include "process/remote.hpp"
#include "vm/state.hpp"

using namespace boost;
using namespace std;
using namespace process;
using namespace vm;
using namespace db;
using namespace utils;

namespace sched
{

void
static_local::assert_end(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
}

void
static_local::assert_end_iteration(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
}

void
static_local::new_work(node *from, node *_to, const simple_tuple *tpl, const bool is_agg)
{
   (void)from;
   thread_node *to((thread_node*)_to);
    
   assert(to != NULL);
   assert(tpl != NULL);
   
   to->add_work(tpl, is_agg);
   
   if(!to->in_queue()) {
      mutex::scoped_lock lock(to->mtx);
      if(!to->in_queue()) {
         add_to_queue(to);
         to->set_in_queue(true);
      }
      // no need to put owner active, since we own this node
      // new_work was called for init or for self generation of
      // tuples (SEND a TO a)
      // the lock is needed in order to make sure
      // the node is not put multiple times on the queue
   }
   
   assert(to->in_queue());
}

void
static_local::new_work_other(sched::base *scheduler, node *node, const simple_tuple *stuple)
{
   assert(is_active());
   assert(node != NULL);
   assert(stuple != NULL);
   assert(scheduler == NULL);
   
   thread_node *tnode((thread_node*)node);
   
   assert(tnode->get_owner() != NULL);
   
   tnode->add_work(stuple, false);
   
   if(!tnode->in_queue()) {
      mutex::scoped_lock lock(tnode->mtx);
      if(!tnode->in_queue()) {
         static_local *owner(tnode->get_owner());
         tnode->set_in_queue(true);
         owner->add_to_queue(tnode);
         
         if(this != owner) {
            mutex::scoped_lock lock2(owner->mutex);
            if(owner->is_inactive())
            {
               if(owner->is_inactive())
                  owner->set_active();
               assert(owner->is_active());
            }
         }
         
         assert(tnode->in_queue());
      }
   }
}

void
static_local::new_work_remote(remote *, const node::node_id, message *)
{
   assert(0);
}

void
static_local::generate_aggs(void)
{
   const node::node_id first(remote::self->find_first_node(id));
   const node::node_id final(remote::self->find_last_node(id));
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(final));

   for(; it != end; ++it)
   {
      node *no(it->second);
      simple_tuple_list ls(no->generate_aggs());

      for(simple_tuple_list::iterator it2(ls.begin());
         it2 != ls.end();
         ++it2)
      {
         new_work(NULL, no, *it2, true);
      }
   }
}

bool
static_local::busy_wait(void)
{
   bool turned_inactive(false);
   
   while(!has_work()) {
      
      if(!turned_inactive) {
         mutex::scoped_lock l(mutex);
         if(!has_work()) {
            if(is_active())
               set_inactive(); // may be inactive from previous iteration
            turned_inactive = true;
            if(all_threads_finished())
               return false;
         }
      }
      
      if(turned_inactive && is_inactive() && all_threads_finished()) {
         assert(turned_inactive);
         assert(is_inactive());
         return false;
      }
   }
   
   // since queue pushing and state setting are done in
   // different exclusive regions, this may be needed
   set_active_if_inactive();
   
   assert(is_active());
   assert(has_work());
   
   return true;
}

bool
static_local::terminate_iteration(void)
{
   // this is needed since one thread can reach set_active
   // and thus other threads waiting for all_finished will fail
   // to get here
   threads_synchronize();

   assert(is_inactive());

   generate_aggs();

   if(has_work())
      set_active();

#ifdef ASSERT_THREADS
   static boost::mutex local_mtx;
   static vector<size_t> total;

   local_mtx.lock();

   total.push_back(iteration);

   if(total.size() == state::NUM_THREADS) {
      for(size_t i(0); i < state::NUM_THREADS; ++i) {
         assert(total[i] == iteration);
      }

      total.clear();
   }

   local_mtx.unlock();
#endif

   // again, needed since we must wait if any thread
   // is set to active in the previous if
   threads_synchronize();

   return !all_threads_finished();
}

void
static_local::finish_work(const work_unit& work)
{
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(current_node->get_owner() == this);
}

bool
static_local::check_if_current_useless(void)
{
   if(current_node->no_more_work()) {
      mutex::scoped_lock lock(current_node->mtx);
      
      if(current_node->no_more_work()) {
         current_node->set_in_queue(false);
         assert(!current_node->in_queue());
         current_node = NULL;
         return true;
      }
   }
   
   assert(!current_node->no_more_work());
   return false;
}

bool
static_local::set_next_node(void)
{
   if(current_node != NULL)
      check_if_current_useless();
   
   while (current_node == NULL) {   
      if(!has_work()) {
         if(!busy_wait())
            return false;
      }
      
      assert(has_work());
      
      current_node = queue_nodes.pop();
      
      assert(current_node != NULL);
      
      check_if_current_useless();
   }
   
   assert(current_node != NULL);
   return true;
}

bool
static_local::get_work(work_unit& work)
{  
   if(!set_next_node())
      return false;
      
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(!current_node->no_more_work());
   
   node_work_unit unit(current_node->get_work());
   
   work.work_tpl = unit.work_tpl;
   work.agg = unit.agg;
   work.work_node = current_node;
   
   assert(work.work_node == current_node);
   
   return true;
}

void
static_local::end(void)
{
}

void
static_local::init(const size_t num_threads)
{
   predicate *init_pred(state::PROGRAM->get_init_predicate());
   
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      thread_node *cur_node((thread_node*)it->second);
      
      cur_node->set_owner(this);
      
      new_work(NULL, cur_node, simple_tuple::create_new(new vm::tuple(init_pred)));
      assert(cur_node->get_owner() == this);
      assert(cur_node->in_queue());
      assert(!cur_node->no_more_work());
   }
   
   threads_synchronize();
}

static_local*
static_local::find_scheduler(const node::node_id id)
{
   return NULL;
}

static_local::static_local(const vm::process_id _id):
   base(_id),
   current_node(NULL)
{
}

static_local::~static_local(void)
{
}
   
vector<sched::base*>&
static_local::start(const size_t num_threads)
{
   init_barriers(num_threads);
   
   for(process_id i(0); i < num_threads; ++i)
      add_thread(new static_local(i));
      
   return ALL_THREADS;
}
   
}