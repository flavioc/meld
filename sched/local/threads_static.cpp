#include <iostream>
#include <boost/thread/barrier.hpp>

#include "sched/local/threads_static.hpp"
#include "db/database.hpp"
#include "db/tuple.hpp"
#include "process/remote.hpp"
#include "sched/thread/assert.hpp"
#include "vm/state.hpp"
#include "sched/common.hpp"

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
   assert_thread_end_iteration();
   assert_static_nodes_end(id);
}

void
static_local::assert_end_iteration(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   assert_static_nodes_end_iteration(id);
}

void
static_local::new_agg(work& new_work)
{
   thread_node *to(dynamic_cast<thread_node*>(new_work.get_node()));
   
   assert_thread_push_work();
   
   node_work node_new_work(new_work);
   to->add_work(node_new_work);
   
   if(!to->in_queue()) {
      add_to_queue(to);
      to->set_in_queue(true);
   }
}

void
static_local::new_work(const node *, work& new_work)
{
   thread_node *to(dynamic_cast<thread_node*>(new_work.get_node()));
   
   assert_thread_push_work();
   //assert(is_active());
   
   node_work node_new_work(new_work);
   
   to->add_work(node_new_work);
   
   if(!to->in_queue()) {
      spinlock::scoped_lock l(to->spin);
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
static_local::new_work_other(sched::base *, work& new_work)
{
   assert(is_active());
   
   thread_node *tnode(dynamic_cast<thread_node*>(new_work.get_node()));
   
   assert(tnode->get_owner() != NULL);
   
   assert_thread_push_work();
   
   node_work node_new_work(new_work);
   tnode->add_work(node_new_work);
   
	spinlock::scoped_lock l(tnode->spin);
   if(!tnode->in_queue() && tnode->has_work()) {
		static_local *owner(dynamic_cast<static_local*>(tnode->get_owner()));
		
		tnode->set_in_queue(true);
		owner->add_to_queue(tnode);

      if(this != owner) {
         spinlock::scoped_lock l2(owner->lock);
         
         if(owner->is_inactive() && owner->has_work())
         {
            owner->set_active();
            assert(owner->is_active());
         }
      } else {
         assert(is_active());
      }
         
      assert(tnode->in_queue());
   }
   
#ifdef INSTRUMENTATION
   sent_facts++;
#endif
}

void
static_local::new_work_remote(remote *, const node::node_id, message *)
{
   assert(false);
}

void
static_local::generate_aggs(void)
{
   iterate_static_nodes(id);
}

bool
static_local::busy_wait(void)
{
   ins_idle;
   
   while(!has_work()) {
      BUSY_LOOP_MAKE_INACTIVE()
      BUSY_LOOP_CHECK_TERMINATION_THREADS()
   }
   
   // since queue pushing and state setting are done in
   // different exclusive regions, this may be needed
   set_active_if_inactive();
   ins_active;
   assert(is_active());
   assert(has_work());
   
   return true;
}

bool
static_local::terminate_iteration(void)
{
   ins_round;
   
   // this is needed since one thread can reach set_active
   // and thus other threads waiting for all_finished will fail
   // to get here
   
   assert_thread_end_iteration();

   assert(is_inactive());
   
   threads_synchronize();

   generate_aggs();

   if(has_work())
      set_active();
   
   assert(total_in_agg > 0);
   total_in_agg--;

   assert_thread_iteration(iteration);
   
   if(leader_thread()) {
      while(total_in_agg != 0) {}
      
#define GET_NEXT(x) ((x) == 1 ? 2 : 1)

      if(num_active() > 0) {
         total_in_agg = state::NUM_THREADS;
         reset_barrier();
         round_state = GET_NEXT(round_state);
         return true;
      } else {
         round_state = 0;
         return false;
      }
   } else {
      const size_t supos(GET_NEXT(thread_round_state));
      
      while(round_state == thread_round_state) {}
      
      if(round_state == supos) {
         thread_round_state = supos;
         assert(thread_round_state == round_state);
         return true;
      } else
         return false;
   }
}

void
static_local::finish_work(const work& work)
{
   base::finish_work(work);
   
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(current_node->get_owner() == this);
}

bool
static_local::check_if_current_useless(void)
{
   if(!current_node->has_work()) {
      spinlock::scoped_lock lock(current_node->spin);
      
      if(!current_node->has_work()) {
         current_node->set_in_queue(false);
         assert(!current_node->in_queue());
         current_node = NULL;
         return true;
      }
   }
   
   assert(current_node->has_work());
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
      
      assert(current_node->in_queue());
      assert(current_node != NULL);
      
      check_if_current_useless();
   }
   
   ins_active;
   
   assert(current_node != NULL);
   
   return true;
}

bool
static_local::get_work(work& new_work)
{  
   if(!set_next_node())
      return false;
      
   set_active_if_inactive();
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(current_node->has_work());
   
   node_work unit(current_node->get_work());
   
   new_work.copy_from_node(current_node, unit);
   
   assert(new_work.get_node() == current_node);
   
   assert_thread_pop_work();
   
   return true;
}

void
static_local::end(void)
{
}

void
static_local::init(const size_t)
{
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      thread_node *cur_node((thread_node*)it->second);
      cur_node->set_owner(this);
      
      init_node(cur_node);
      
      assert(cur_node->get_owner() == this);
      assert(cur_node->in_queue());
      assert(cur_node->has_work());
   }
   
   threads_synchronize();
}

static_local*
static_local::find_scheduler(const node *n)
{
   return (static_local*)ALL_THREADS[remote::self->find_proc_owner(n->get_id())];
}

void
static_local::write_slice(stat::slice& sl) const
{
#ifdef INSTRUMENTATION
   base::write_slice(sl);
   sl.work_queue = queue_nodes.size();
#else
   (void)sl;
#endif
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
