
#include "sched/mpi/handler.hpp"
#include "vm/state.hpp"
#include "process/router.hpp"

using namespace std;
using namespace vm;
using namespace db;

#ifdef IMPLEMENT_MISSING_MPI
namespace MPI {

   void Comm::Set_errhandler(MPI::Errhandler const&) {
      assert(0);
   }

   void Win::Set_errhandler(MPI::Errhandler const&) {
      assert(0);
   }
}
#endif

namespace sched
{
   
void
mpi_handler::fetch_work(void)
{
   if(!state::ROUTER->use_mpi())
      return;
   
   size_t total_received(0);
   message_set *ms;
      
   while((ms = state::ROUTER->recv_attempt(0)) != NULL) {
      assert(!ms->empty());
      
      for(list_messages::const_iterator it(ms->begin()); it != ms->end(); ++it) {
         message *msg(*it);
         node *node(state::DATABASE->find_node(msg->id));
         simple_tuple *tpl(msg->data);

         node->more_to_process(tpl->get_predicate_id());
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
	   else if(step_fetch < MPI_MAX_ROUND_TRIP_FETCH)
		   step_fetch += MPI_INCREASE_ROUND_TRIP_FETCH;

      step_fetch = max(step_fetch,MPI_MIN_ROUND_TRIP_FETCH);
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
      fetch_work();
      round_trip_fetch = 0;
   }
}

void
mpi_handler::update_pending_messages(const bool test)
{
   msg_buf.update_received(test);
}
   
mpi_handler::mpi_handler(void):
   step_fetch(MPI_DEFAULT_ROUND_TRIP_FETCH),
   step_send(MPI_DEFAULT_ROUND_TRIP_SEND),
   round_trip_fetch(0),
   round_trip_update(0),
   round_trip_send(0)
{  
}

}
