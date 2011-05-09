#include <iostream>
#include <boost/thread/barrier.hpp>

#include "sched/stealer.hpp"
#include "db/database.hpp"
#include "db/tuple.hpp"
#include "process/remote.hpp"
#include "vm/state.hpp"
#include "utils/utils.hpp"
#include "sched/termination_barrier.hpp"

using namespace boost;
using namespace std;
using namespace process;
using namespace vm;
using namespace db;
using namespace utils;

namespace sched
{

static vector<stealer*> others;
static barrier *thread_barrier(NULL);
static termination_barrier* term_barrier(NULL);

static inline void
threads_synchronize(void)
{
   thread_barrier->wait();
}

void
stealer::assert_end(void) const
{
   assert(queue_nodes.empty());
   assert(process_state == PROCESS_INACTIVE);
   assert(term_barrier->all_finished());
}

void
stealer::assert_end_iteration(void) const
{
   assert(queue_nodes.empty());
   assert(process_state == PROCESS_INACTIVE);
   assert(term_barrier->all_finished());
}

void
stealer::make_active(void)
{
   if(process_state == PROCESS_INACTIVE) {
      term_barrier->is_active();
      process_state = PROCESS_ACTIVE;
#ifdef DEBUG_ACTIVE
      cout << "Active " << id << endl;
#endif
   }
}

void
stealer::make_inactive(void)
{
   if(process_state == PROCESS_ACTIVE) {
      term_barrier->is_inactive();
      process_state = PROCESS_INACTIVE;
#ifdef DEBUG_ACTIVE
      cout << "Inactive: " << id << endl;
#endif
   }
}

void
stealer::new_work(node *from, node *_to, const simple_tuple *tpl, const bool is_agg)
{
   (void)from;
   thread_node *to((thread_node*)_to);
    
   assert(to != NULL);
   assert(tpl != NULL);
   
   to->add_work(tpl, is_agg);
   
   if(!to->in_queue())
      add_to_queue(to);
   
   assert(to->in_queue());
}

void
stealer::new_work_other(sched::base *scheduler, node *node, const simple_tuple *stuple)
{
   assert(process_state == PROCESS_ACTIVE);
   assert(node != NULL);
   assert(stuple != NULL);
   assert(scheduler == NULL);
   
   thread_node *tnode((thread_node*)node);
   
   tnode->add_work(stuple, false);
   
   if(!tnode->in_queue() && !tnode->no_more_work()) {
      mutex::scoped_lock lock(tnode->mtx);
      if(!tnode->in_queue() && !tnode->no_more_work()) {
         tnode->set_in_queue(true);
         stealer *owner(tnode->get_owner());
         {
            mutex::scoped_lock lock2(owner->mutex);
            owner->make_active();
            owner->add_to_queue(tnode);
            assert(owner->process_state == PROCESS_ACTIVE);
         }
         assert(tnode->in_queue());
      }
   }
}

void
stealer::new_work_remote(remote *, const vm::process_id, message *)
{
   assert(0);
}

void
stealer::generate_aggs(void)
{
   for(node_set::iterator it(nodes->begin()); it != nodes->end(); ++it) {
      node *no(*it);
      simple_tuple_list ls(no->generate_aggs());

      for(simple_tuple_list::iterator it2(ls.begin());
         it2 != ls.end();
         ++it2)
      {
         //cout << no->get_id() << " GENERATE " << **it2 << endl;
         new_work(NULL, no, *it2, true);
      }
   }
}

stealer*
stealer::select_steal_target(void) const
{
   size_t idx(random_unsigned(others.size()));
   
   while(others[idx] == this)
      idx = random_unsigned(others.size());
   
   return others[idx];
}

bool
stealer::busy_wait(void)
{
   bool turned_inactive(false);
   
   while(queue_nodes.empty()) {
      
      if(!turned_inactive) {
         mutex::scoped_lock l(mutex);
         if(queue_nodes.empty() && process_state == PROCESS_ACTIVE) {
            make_inactive();
            turned_inactive = true;
            if(term_barrier->all_finished())
               return false;
         } else if(process_state == PROCESS_INACTIVE && queue_nodes.empty()) {
            turned_inactive = true;
         }
      }
      
      if(term_barrier->all_finished()) {
         assert(process_state == PROCESS_INACTIVE);
         return false;
      }
   }
   
   assert(process_state == PROCESS_ACTIVE);
   assert(!queue_nodes.empty());
   
   return true;
}

bool
stealer::terminate_iteration(void)
{
   // this is needed since one thread can reach make_active
   // and thus other threads waiting for all_finished will fail
   // to get here
   threads_synchronize();

   assert(process_state == PROCESS_INACTIVE);

   generate_aggs();

   if(!queue_nodes.empty()) {
      make_active();
   }

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

   return !term_barrier->all_finished();
}

void
stealer::finish_work(const work_unit& work)
{
   assert(current_node != NULL);
   assert(current_node->in_queue());
}

bool
stealer::check_if_current_useless(void)
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
stealer::set_next_node(void)
{
   if(current_node != NULL)
      check_if_current_useless();
   
   while (current_node == NULL) {   
      if(queue_nodes.empty()) {
         if(!busy_wait())
            return false;
      }
      
      assert(!queue_nodes.empty());
      
      current_node = queue_nodes.pop();
      
      assert(current_node != NULL);
      
      check_if_current_useless();
   }
   
   assert(current_node != NULL);
   return true;
}

bool
stealer::get_work(work_unit& work)
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
stealer::end(void)
{
   delete nodes_mutex;
}

void
stealer::add_node(node *node)
{
   mutex::scoped_lock l(*nodes_mutex);
   
   nodes->insert(node);
}

void
stealer::remove_node(node *node)
{
   mutex::scoped_lock l(*nodes_mutex);
   
   nodes->erase(node);
}

void
stealer::init(const size_t num_threads)
{  
   nodes_mutex = new boost::mutex();
   nodes = new node_set();
   
   predicate *init_pred(state::PROGRAM->get_init_predicate());
   
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      thread_node *cur_node((thread_node*)it->second);
      cur_node->set_owner(this);
      
      new_work(NULL, cur_node, simple_tuple::create_new(new vm::tuple(init_pred)));
      
      nodes->insert(cur_node);
   }
   
   threads_synchronize();
}

stealer*
stealer::find_scheduler(const node::node_id id)
{
   return NULL;
}

stealer::stealer(const vm::process_id _id):
   base(_id),
   process_state(PROCESS_ACTIVE),
   current_node(NULL),
   nodes(NULL)
{
}

stealer::~stealer(void)
{
   delete nodes;
}
   
vector<stealer*>&
stealer::start(const size_t num_threads)
{
   thread_barrier = new barrier(num_threads);
   term_barrier = new termination_barrier(num_threads);
   others.resize(num_threads);
   
   for(process_id i(0); i < num_threads; ++i)
      others[i] = new stealer(i);
      
   return others;
}
   
}