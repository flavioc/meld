
#include "thread/mpi_static.hpp"
#include "vm/state.hpp"
#include "process/router.hpp"
#include "utils/utils.hpp"

using namespace std;
using namespace vm;
using namespace process;
using namespace db;
using namespace boost;
using namespace utils;

namespace sched
{

void
mpi_thread_static::assert_end(void) const
{
   static_local::assert_end();
   assert_mpi();
}

void
mpi_thread_static::assert_end_iteration(void) const
{
   static_local::assert_end_iteration();
   assert_mpi();
}

void
mpi_thread_static::new_mpi_message(node *_node, simple_tuple *stpl)
{
   assert(remote::self->find_proc_owner(_node->get_id()) == get_id());
   
   thread_node *node(dynamic_cast<thread_node*>(_node));
   
   assert(node->get_owner() == this);
   
   node_work unit(stpl);
   
   node->add_work(unit);
   
   if(!node->in_queue() && node->has_work()) {
      spinlock::scoped_lock l(node->spin);

      if(!node->in_queue() && node->has_work()) {
         node->set_in_queue(true);
         this->add_to_queue(node);
      }
   }
   
   set_active_if_inactive();
   
   assert(node->in_queue());
   assert(is_active());
}

bool
mpi_thread_static::busy_wait(void)
{
   ins_sched;
   
   IDLE_MPI_ALL(get_id())
   
   ins_idle;
      
   while(!has_work()) {
      BUSY_LOOP_MAKE_INACTIVE()
      
      if(attempt_token(term_barrier, leader_thread())) {
         assert(all_threads_finished());
         assert(is_inactive());
         assert(!has_work());
         assert(iteration_finished);
         return false;
      }
      
      fetch_work(get_id());
   }
   
   set_active_if_inactive();
   
   assert(is_active());
   assert(has_work());
   
   return true;
}

void
mpi_thread_static::new_work_remote(remote *rem, const node::node_id node_id, message *msg)
{
   const process_id target(rem->find_proc_owner(node_id));
   
   assert(rem != remote::self);
   
   buffer_message(rem, target, msg);
}

bool
mpi_thread_static::get_work(work& new_work)
{  
   do_mpi_cycle(get_id());
   
   return static_local::get_work(new_work);
}

bool
mpi_thread_static::terminate_iteration(void)
{
   if(leader_thread())
      token.token_terminate_iteration();

   assert(iteration_finished);
   
   update_pending_messages(false); // just delete all requests
   
   START_ROUND();
   
   if(has_work())
      set_active();
   
   END_ROUND(
      more_work = state::ROUTER->reduce_continue(num_active() > 0);
      iteration_finished = !more_work;
   );
}

}
