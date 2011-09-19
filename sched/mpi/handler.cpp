
#include <boost/thread/barrier.hpp>

#include "sched/mpi/handler.hpp"
#include "vm/state.hpp"
#include "process/router.hpp"

using namespace std;
using namespace vm;
using namespace db;
using namespace process;
using namespace utils;

namespace sched
{

volatile bool mpi_handler::iteration_finished(false);
tokenizer mpi_handler::token;
boost::mutex mpi_handler::tok_mutex;
atomic<size_t> mpi_handler::inside_counter(0);
static boost::barrier *inside_barrier(NULL);
   
void
mpi_handler::fetch_work(const process_id id)
{
   if(!state::ROUTER->use_mpi())
      return;
   
   size_t total_received(0);
   message_set *ms;
      
   while((ms = state::ROUTER->recv_attempt(id, recv_buf)) != NULL) {
      assert(!ms->empty());
      
      for(list_messages::const_iterator it(ms->begin()); it != ms->end(); ++it) {
         message *msg(*it);
         node *node(state::DATABASE->find_node(msg->id));
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
mpi_handler::transmit_messages(void)
{
   const size_t transmitted(msg_buf.transmit());
   
   if(transmitted > 0)
      messages_were_transmitted(transmitted);
}

void
mpi_handler::do_mpi_worker_cycle(void)
{
   ++round_trip_update;
   ++round_trip_send;

   if(round_trip_update == MPI_ROUND_TRIP_UPDATE) {
      update_pending_messages(true);
      round_trip_update = 0;
   }

   if(round_trip_send == step_send) {
      transmit_messages();
      round_trip_send = 0;
   }
}

void
mpi_handler::do_mpi_leader_cycle(void)
{
   ++round_trip_fetch;
   
   if(round_trip_fetch == step_fetch) {
      fetch_work(0);
      round_trip_fetch = 0;
   }
}

void
mpi_handler::do_mpi_cycle(const process_id id)
{
   do_mpi_worker_cycle();
   
   ++round_trip_fetch;
   
   if(round_trip_fetch == step_fetch) {
      fetch_work(id);
      round_trip_fetch = 0;
   }
}

void
mpi_handler::update_pending_messages(const bool test)
{
   msg_buf.update_received(test);
}

bool
mpi_handler::attempt_token(termination_barrier *barrier, const bool main)
{
   if(!barrier->zero_active_threads())
      return false;
   if(iteration_finished)
      return true;

   inside_counter--;
   
   while(inside_counter != 0) {
      if(!barrier->zero_active_threads() || iteration_finished) {
         //printf("HERE fail\n");
         inside_counter++;
         return iteration_finished;
      }
   }
   
   assert(inside_counter == 0);
   assert(barrier->zero_active_threads());
   
   inside_barrier->wait();
   
   //printf("HERE\n");
   
   // all threads are now here!
   if(main) {
      // HERE
      boost::mutex::scoped_lock lock(tok_mutex);
      
      if(!token.busy_loop_token(true)) {
         assert(barrier->zero_active_threads());
         barrier->set_done();
         iteration_finished = true;
      }
      
      assert(inside_counter == 0);
      inside_counter = state::NUM_THREADS;
   }
   
   //printf("Gonna wait\n");
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