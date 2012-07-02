
#include "thread/mpi_single.hpp"
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
   ins_sched;
   IDLE_MPI()
      
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
   update_pending_messages(false); // just delete all requests
   
   if(leader_thread())
      token.token_terminate_iteration();

   assert(iteration_finished);
   
   threads_synchronize();
   START_ROUND();
   
   DO_END_ROUND(
      more_work = state::ROUTER->reduce_continue(has_work());
      iteration_finished = !more_work;
   ,
      set_active();
   );
}

vector<sched::base*>&
mpi_thread_single::start(const size_t num_threads)
{
   threads_single::start_base(num_threads);
   assert_thread_disable_work_count();
   
   for(process_id i(0); i < num_threads; ++i)
      add_thread(new mpi_thread_single(i));
   
   return ALL_THREADS;
}

}
