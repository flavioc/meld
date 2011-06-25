
#include <functional>

#include "sched/global/mpi.hpp"

#ifdef COMPILE_MPI

#include <mpi.h>

#include "vm/state.hpp"
#include "process/router.hpp"
#include "utils/utils.hpp"

using namespace db;
using namespace process;
using namespace vm;
using namespace std;
using namespace utils;
using namespace boost;

namespace sched
{
   
void
mpi_static::assert_end_iteration(void)
{
   static_global::assert_end_iteration();
   assert_mpi();
}

void
mpi_static::assert_end(void)
{
   static_global::assert_end();
   assert_mpi();
}

bool
mpi_static::get_work(work_unit& work)
{
   MPI_WORK_CYCLE()
   
   return static_global::get_work(work);
}

void
mpi_static::new_mpi_message(node *node, simple_tuple *stpl)
{
   mpi_static *other((mpi_static*)find_scheduler(node->get_id()));

   if(other == this) {
      set_active_if_inactive();
      new_work(NULL, node, stpl);
      assert(is_active());
   } else {
      new_work_other(other, node, stpl);
   }
}

bool
mpi_static::busy_wait(void)
{
   flush_buffered();
   IDLE_MPI()
   
   while(!has_work()) {
      BUSY_LOOP_MAKE_INACTIVE()
      
      /*BUSY_LOOP_CHECK_INACTIVE_THREADS()
      BUSY_LOOP_CHECK_INACTIVE_MPI()*/
      
      boost::function0<bool> f(boost::bind(&mpi_static::all_threads_finished, this));
      
      if(attempt_token(f, leader_thread())) {
         assert(all_threads_finished());
         assert(is_inactive());
         assert(!has_work());
         assert(iteration_finished);
         return false;
      }
      
      BUSY_LOOP_FETCH_WORK()
      if(leader_thread())
         flush_buffered();
   }
   
   set_active_if_inactive();
   
   assert(has_work());
   
   return true;
}

bool
mpi_static::terminate_iteration(void)
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

void
mpi_static::new_work_remote(remote *rem, const node::node_id, message *msg)
{
   // this is buffered as the 0 thread id, since only one thread per proc exists
   buffer_message(rem, 0, msg);
}

#endif

}
