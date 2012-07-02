#include <iostream>
#include <boost/thread/barrier.hpp>

#include "thread/single.hpp"
#include "db/database.hpp"
#include "db/tuple.hpp"
#include "process/remote.hpp"
#include "sched/thread/assert.hpp"
#include "vm/state.hpp"
#include "utils/time.hpp"
#include "sched/common.hpp"

using namespace boost;
using namespace std;
using namespace process;
using namespace vm;
using namespace db;
using namespace utils;

namespace sched
{

queue::safe_linear_queue<thread_node*> *threads_single::queue_nodes(NULL);

void
threads_single::assert_end(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   assert_static_nodes_end(id);
}

void
threads_single::assert_end_iteration(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   assert_static_nodes_end_iteration(id);
}

void
threads_single::new_agg(work& new_work)
{
   thread_node *to(dynamic_cast<thread_node*>(new_work.get_node()));
   
   assert(to != NULL);
   
   assert_thread_push_work();
   
   node_work new_node_work(new_work);
   to->add_work(new_node_work);
   
   if(!to->in_queue()) {
      to->set_in_queue(true);
      add_to_queue(to);
   }
}

void
threads_single::new_work(const node *, work& new_work)
{
   thread_node *to(dynamic_cast<thread_node*>(new_work.get_node()));
    
   assert(to != NULL);
   assert(is_active());
   
   assert_thread_push_work();
   
   node_work new_node_work(new_work);
   to->add_work(new_node_work);
   
   if(!to->in_queue() && to->has_work()) {
      spinlock::scoped_lock l(to->spin);
      if(!to->in_queue() && to->has_work()) {
         to->set_in_queue(true);
         add_to_queue(to);
      }
   }
}

void
threads_single::new_work_other(sched::base *scheduler, work& new_work)
{
   assert(scheduler == NULL);
   assert(is_active());
   
   thread_node *tnode(dynamic_cast<thread_node*>(new_work.get_node()));
   
   assert_thread_push_work();
   
   node_work new_node_work(new_work);
   
   tnode->add_work(new_node_work);
   
   {
      spinlock::scoped_lock l(tnode->spin);
      
      if(!tnode->in_queue() && tnode->has_work()) {
         tnode->set_in_queue(true);
         add_to_queue(tnode);
      }
   }
}

void
threads_single::new_work_remote(remote *, const node::node_id, message *)
{
   assert(false);
}

void
threads_single::generate_aggs(void)
{
   iterate_static_nodes(id);
}

bool
threads_single::busy_wait(void)
{
   ins_idle;
   
   while(!has_work()) {
      BUSY_LOOP_MAKE_INACTIVE()
      BUSY_LOOP_CHECK_TERMINATION_THREADS()
   }
   
   // since queue pushing and state setting are done in
   // different exclusive regions, this may be needed
   set_active_if_inactive();
   
   assert(is_active());
   
   return true;
}

bool
threads_single::terminate_iteration(void)
{
   threads_synchronize();
   
   START_ROUND();
   
   DO_END_ROUND(
      more_work = has_work();
   ,
      set_active();
   );
}

void
threads_single::finish_work(const work& new_work)
{
   base::finish_work(new_work);
   
   assert(current_node != NULL);
   assert(current_node->in_queue());
}

bool
threads_single::check_if_current_useless(void)
{
   if(!current_node->has_work()) {
      spinlock::scoped_lock l(current_node->spin);
      
      if(!current_node->has_work()) {
         current_node->set_in_queue(false);
         assert(!current_node->in_queue());
         current_node = NULL;
         return true;
      }
   }
   
   return false;
}

bool
threads_single::set_next_node(void)
{
   if(current_node != NULL)
      check_if_current_useless();
   
   while (current_node == NULL) {   
      if(!has_work()) {
         if(!busy_wait())
            return false;
      }
            
      if(!queue_nodes->pop(current_node))
         continue;
      
      assert(current_node->in_queue());
      assert(current_node != NULL);
      
      check_if_current_useless();
   }
   
   assert(current_node != NULL);
   return true;
}

bool
threads_single::get_work(work& new_work)
{  
   if(!set_next_node())
      return false;
      
   ins_active;
   
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
threads_single::init(const size_t)
{
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      thread_node *cur_node((thread_node*)it->second);
      
      init_node(cur_node);
      
      assert(cur_node->in_queue());
      assert(cur_node->has_work());
   }
   
   threads_synchronize();
}

threads_single*
threads_single::find_scheduler(const node *)
{
   return NULL;
}

void
threads_single::write_slice(statistics::slice& sl) const
{
#ifdef INSTRUMENTATION
   base::write_slice(sl);
   sl.work_queue = queue_nodes->size();
#else
   (void)sl;
#endif
}

threads_single::threads_single(const vm::process_id _id):
   base(_id),
   current_node(NULL)
{
}

threads_single::~threads_single(void)
{
}

void
threads_single::start_base(const size_t num_threads)
{
   init_barriers(num_threads);
   queue_nodes = new queue::safe_linear_queue<thread_node*>();
}
   
vector<sched::base*>&
threads_single::start(const size_t num_threads)
{
   start_base(num_threads);
   
   for(process_id i(0); i < num_threads; ++i)
      add_thread(new threads_single(i));

   return ALL_THREADS;
}

}
