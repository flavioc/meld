
#include "sched/local/mpi_threads_single.hpp"
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
mpi_thread_single::assert_end(void) const
{
   threads_single::assert_end();
   assert_mpi();
}

void
mpi_thread_single::assert_end_iteration(void) const
{
   threads_single::assert_end_iteration();
   assert_mpi();
}

void
mpi_thread_single::new_mpi_message(node *_node, simple_tuple *stpl)
{
   thread_node *node(dynamic_cast<thread_node*>(_node));
   
   node_work unit(stpl);
   
   node->add_work(unit);
   
   spinlock::scoped_lock l(node->spin);
   
   if(!node->in_queue() && node->has_work()) {
      node->set_in_queue(true);
      add_to_queue(node);
   }
}

bool
mpi_thread_single::busy_wait(void)
{
   IDLE_MPI()
      
   while(!has_work()) {
      BUSY_LOOP_MAKE_INACTIVE()
      BUSY_LOOP_CHECK_INACTIVE_THREADS()
      BUSY_LOOP_CHECK_INACTIVE_MPI()
      BUSY_LOOP_FETCH_WORK()
   }
   
   set_active_if_inactive();
   
   assert(is_active());
   
   return true;
}

void
mpi_thread_single::new_work_remote(remote *rem, const node::node_id, message *msg)
{
   buffer_message(rem, 0, msg);
}

bool
mpi_thread_single::get_work(work& new_work)
{
   MPI_WORK_CYCLE()
   
   return threads_single::get_work(new_work);
}

bool
mpi_thread_single::terminate_iteration(void)
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
   
   assert_thread_iteration(iteration);

   // again, needed since we must wait if any thread
   // is set to active in the previous if
   threads_synchronize();
   
   if(leader_thread()) {
      const bool more_work = state::ROUTER->reduce_continue(has_work());
      iteration_finished = !more_work;
   }
   
   // threads must wait for the final answer between processes
   threads_synchronize();

   const bool ret(!iteration_finished);
   
   if(ret)
      set_active();
   
   threads_synchronize();
   
   return ret;
}

}
