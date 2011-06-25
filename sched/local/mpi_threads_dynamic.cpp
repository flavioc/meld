
#include "sched/local/mpi_threads_dynamic.hpp"
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
mpi_thread_dynamic::assert_end(void) const
{
   dynamic_local::assert_end();
   assert(iteration_finished);
   assert_mpi();
}

void
mpi_thread_dynamic::assert_end_iteration(void) const
{
   dynamic_local::assert_end_iteration();
   assert(iteration_finished);
   assert_mpi();
}

void
mpi_thread_dynamic::new_mpi_message(node *_node, simple_tuple *stpl)
{
   thread_node *node((thread_node*)_node);
   spinlock::scoped_lock l(node->spin);

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
      mpi_thread_dynamic *owner = (mpi_thread_dynamic*)node->get_owner();
   
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

void
mpi_thread_dynamic::change_node(thread_node *node, dynamic_local *_asker)
{
   mpi_thread_dynamic *asker((mpi_thread_dynamic*)_asker);
   
   assert(node != current_node);
   assert(node->get_owner() == this);
   
   remove_node(node);
   asker->add_node(node);
   
   {
      spinlock::scoped_lock lock(node->spin);
      node->set_owner((static_local*)asker);
      assert(node->in_queue());
      assert(node->get_owner() == asker);
   }
   
   asker->add_to_queue(node);
}

bool
mpi_thread_dynamic::busy_wait(void)
{
   size_t asked_many(0);
   boost::function0<bool> f(boost::bind(&mpi_thread_dynamic::all_threads_finished, this));
   
   IDLE_MPI_ALL(get_id())
   
   while(!has_work()) {
      
      if(is_inactive() && state::NUM_THREADS > 1 && asked_many < MAX_ASK_STEAL) {
         mpi_thread_dynamic *target((mpi_thread_dynamic*)select_steal_target());
         
         if(target->is_active()) {
            target->request_work_to(this);
            ++asked_many;
         }
      }
      
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
mpi_thread_dynamic::new_work_remote(remote *rem, const node::node_id node_id, message *msg)
{
   const process_id target(rem->find_proc_owner(node_id));
   
   assert(rem != remote::self);
   
   buffer_message(rem, target, msg);
}

bool
mpi_thread_dynamic::get_work(work_unit& work)
{  
   do_mpi_cycle(get_id());
   
   return dynamic_local::get_work(work);
}

bool
mpi_thread_dynamic::terminate_iteration(void)
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
