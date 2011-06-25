
#include "sched/local/mpi_threads_static.hpp"
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
   
   thread_node *node((thread_node*)_node);
   
   assert(node->get_owner() == this);
   
   node->add_work(stpl, false);
   
   if(!node->in_queue()) {
      spinlock::scoped_lock l(node->spin);

      if(!node->in_queue()) {
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
   boost::function0<bool> f(boost::bind(&mpi_thread_static::all_threads_finished, this));
   IDLE_MPI_ALL(get_id())
      
   while(!has_work()) {
      BUSY_LOOP_MAKE_INACTIVE()
      
      if(attempt_token(f, leader_thread())) {
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
mpi_thread_static::get_work(work_unit& work)
{  
   do_mpi_cycle(get_id());
   
   return static_local::get_work(work);
}

bool
mpi_thread_static::terminate_iteration(void)
{
   // this is needed since one thread can reach set_active
   // and thus other threads waiting for all_finished will fail
   // to get here
   threads_synchronize();
   update_pending_messages(false); // just delete all requests
   
   if(leader_thread())
      token.token_terminate_iteration();

   assert(iteration_finished);
   assert(is_inactive());

   generate_aggs();

   if(has_work())
      set_active();
   
   assert_thread_iteration(iteration);

   // again, needed since we must wait if any thread
   // is set to active in the previous if
   threads_synchronize();
   
   if(leader_thread()) {
      const bool more_work = state::ROUTER->reduce_continue(!all_threads_finished());
      iteration_finished = !more_work;
   }
   
   // threads must wait for the final answer between processes
   threads_synchronize();

   const bool ret(!iteration_finished);
   
   threads_synchronize();
   
   return ret;
}

}
