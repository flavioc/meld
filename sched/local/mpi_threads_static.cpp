
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
   thread_node *node((thread_node*)_node);
   spinlock::scoped_lock lnode(node->spin);

   if(node->get_owner() == this) {
      node->add_work(stpl, false);
      if(!node->in_queue()) {
         node->set_in_queue(true);
         this->add_to_queue(node);
      }
      set_active_if_inactive();
      assert(node->in_queue());
      assert(is_active());
   } else {
      mpi_thread_static *owner = (mpi_thread_static*)node->get_owner();
   
      node->add_work(stpl, false);
   
      if(!node->in_queue()) {
         node->set_in_queue(true);
         owner->add_to_queue(node);
      }
   
      assert(owner != NULL);
   
      if(owner->is_inactive()) {
         spinlock::scoped_lock lock(owner->lock);
         if(owner->is_inactive() && owner->has_work())
            owner->set_active();
      }
   }
}

bool
mpi_thread_static::busy_wait(void)
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
   assert(has_work());
   
   return true;
}

void
mpi_thread_static::new_work_remote(remote *rem, const node::node_id, message *msg)
{
   buffer_message(rem, 0, msg);
}

bool
mpi_thread_static::get_work(work_unit& work)
{  
   MPI_WORK_CYCLE()
   
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
