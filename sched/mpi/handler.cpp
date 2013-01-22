
#include <boost/thread/barrier.hpp>

#include "sched/mpi/handler.hpp"
#include "vm/state.hpp"
#include "process/router.hpp"

using namespace std;
using namespace vm;
using namespace db;
using namespace process;
using namespace utils;

#ifdef COMPILE_MPI

namespace sched
{

volatile bool mpi_handler::iteration_finished(false);
tokenizer mpi_handler::token;
boost::mutex mpi_handler::tok_mutex;
atomic<size_t> mpi_handler::inside_counter(0);
static boost::barrier *inside_barrier(NULL);
   
void
mpi_handler::fetch_work(const process_id id, vm::all *all)
{
   if(!all->ROUTER->use_mpi())
      return;
   
   size_t total_received(0);
   message_set *ms;
      
   while((ms = all->ROUTER->recv_attempt(id, recv_buf, all->PROGRAM)) != NULL) {
      assert(!ms->empty());
      
      for(list_messages::const_iterator it(ms->begin()); it != ms->end(); ++it) {
         message *msg(*it);
         node *node(all->DATABASE->find_node(msg->id));
         simple_tuple *tpl(msg->data);
         
         assert(msg->id == node->get_id());

         new_mpi_message(node, tpl);
         
         delete msg;
      }
      
      ++total_received;
         
#ifdef DEBUG_REMOTE
      cout << "Received " << ms->size() << " works" << endl;
#endif
   }
   
   if(total_received > 0) {
      messages_were_received(total_received);
   
      if(step_fetch > MPI_MIN_ROUND_TRIP_FETCH)
		   step_fetch -= MPI_DECREASE_ROUND_TRIP_FETCH;
		
		step_fetch = max(step_fetch, MPI_MIN_ROUND_TRIP_FETCH);
	} else {
	   if(step_fetch < MPI_MAX_ROUND_TRIP_FETCH)
		   step_fetch += MPI_INCREASE_ROUND_TRIP_FETCH;
   }
   
   assert(ms == NULL);
}

void
mpi_handler::transmit_messages(vm::all *all)
{
   const size_t transmitted(msg_buf.transmit(all));
   
   if(transmitted > 0)
      messages_were_transmitted(transmitted);
}

void
mpi_handler::do_mpi_worker_cycle(vm::all *all)
{
   ++round_trip_update;
   ++round_trip_send;

   if(round_trip_update == MPI_ROUND_TRIP_UPDATE) {
      update_pending_messages(true, all);
      round_trip_update = 0;
   }

   if(round_trip_send == step_send) {
      transmit_messages(all);
      round_trip_send = 0;
   }
}

void
mpi_handler::do_mpi_leader_cycle(vm::all *all)
{
   ++round_trip_fetch;
   
   if(round_trip_fetch == step_fetch) {
      fetch_work(0, all);
      round_trip_fetch = 0;
   }
}

void
mpi_handler::do_mpi_cycle(const process_id id, vm::all *all)
{
   do_mpi_worker_cycle(all);
   
   ++round_trip_fetch;
   
   if(round_trip_fetch == step_fetch) {
      fetch_work(id, all);
      round_trip_fetch = 0;
   }
}

void
mpi_handler::update_pending_messages(const bool test, vm::all *all)
{
   msg_buf.update_received(test, all);
}

bool
mpi_handler::attempt_token(termination_barrier *barrier, const bool main, vm::all *all)
{
   if(!barrier->zero_active_threads())
      return false;
   if(iteration_finished)
      return true;

   inside_counter--;
   
   while(inside_counter != 0) {
      if(!barrier->zero_active_threads() || iteration_finished) {
         inside_counter++;
         return iteration_finished;
      }
   }
   
   assert(inside_counter == 0);
   assert(barrier->zero_active_threads());
   
   inside_barrier->wait();
   
   if(main) {
      boost::mutex::scoped_lock lock(tok_mutex);
      
      if(!token.busy_loop_token(true, all)) {
         assert(barrier->zero_active_threads());
         barrier->set_done();
         iteration_finished = true;
      }
      
      assert(inside_counter == 0);
      inside_counter = all->NUM_THREADS;
   }
   
   inside_barrier->wait();
   
   return iteration_finished;
}
   
mpi_handler::mpi_handler(void):
   step_fetch(MPI_DEFAULT_ROUND_TRIP_FETCH),
   step_send(MPI_DEFAULT_ROUND_TRIP_SEND),
   round_trip_fetch(0),
   round_trip_update(0),
   round_trip_send(0)
{  
}

void
mpi_handler::init(const size_t num_threads)
{
   token.init();
   inside_counter = num_threads;
   inside_barrier = new boost::barrier(num_threads);
}

void
mpi_handler::end(void)
{
   token.end();
}

}
#endif
