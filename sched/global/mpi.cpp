
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
mpi_static::get_work(work& new_work)
{
   do_mpi_cycle(get_id());
   
   return static_global::get_work(new_work);
}

void
mpi_static::new_mpi_message(node *node, simple_tuple *stpl)
{
   assert(remote::self->find_proc_owner(node->get_id()) == get_id());

   set_active_if_inactive();
   
   work w(node, stpl);
   
   new_work(NULL, w);
   
   assert(is_active());
}

bool
mpi_static::busy_wait(void)
{
   boost::function0<bool> f(boost::bind(&mpi_static::all_threads_finished, this));
   
   flush_buffered();
   ins_idle;
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
   
   ins_active;
   set_active_if_inactive();
   
   assert(has_work());
   
   return true;
}

bool
mpi_static::terminate_iteration(void)
{
   ins_round;
   
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
      const bool we_have_work(num_active() > 0);
      const bool more_work = state::ROUTER->reduce_continue(we_have_work);
      iteration_finished = !more_work;
      
      if(more_work) {
         if(!we_have_work)
            set_active();
         reset_barrier();
      }
   }
   
   // threads must wait for the final answer between processes
   threads_synchronize();

   const bool ret(!iteration_finished);
   
   threads_synchronize();
   
   return ret;
}

void
mpi_static::new_work_remote(remote *rem, const node::node_id node_id, message *msg)
{
   const process_id target(rem->find_proc_owner(node_id));
   
   assert(rem != remote::self);
   
   //cout << "node " << (int)node_id << " to thread " << (int)target << " in " << rem->get_rank() << endl;
   
   buffer_message(rem, target, msg);
}

#endif

}
