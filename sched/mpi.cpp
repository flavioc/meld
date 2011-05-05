
#include "sched/mpi.hpp"

#ifdef COMPILE_MPI

#include <mpi.h>

#include "vm/state.hpp"
#include "process/router.hpp"

using namespace db;
using namespace process;
using namespace vm;

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
mpi_static::assert_end_iteration(void)
{
   sstatic::assert_end_iteration();
   
   state::ROUTER->update_sent_states();
   
   assert(msg_buf.empty());
   assert(msg_buf.all_received());
   assert(state::ROUTER->all_states_sent());
}

void
mpi_static::assert_end(void)
{
   sstatic::assert_end();
   
   assert(msg_buf.empty());
   assert(msg_buf.all_received());
   assert(state::ROUTER->all_states_sent());
}
   
void
mpi_static::begin_get_work(void)
{
   static const size_t ROUND_TRIP_FETCH_MPI(40);
   static const size_t ROUND_TRIP_UPDATE_MPI(40);
   static const size_t ROUND_TRIP_SEND_MPI(40);
   
   ++round_trip_fetch;
   ++round_trip_update;
   ++round_trip_send;
   
   if(round_trip_fetch == ROUND_TRIP_FETCH_MPI) {
      fetch_work();
      round_trip_fetch = 0;
   }
   
   if(round_trip_update == ROUND_TRIP_UPDATE_MPI) {
      update_pending_messages();
      round_trip_update = 0;
   }
   
   if(round_trip_send == ROUND_TRIP_SEND_MPI) {
      msg_buf.transmit();
      round_trip_send = 0;
   }
}

void
mpi_static::fetch_work(void)
{
   if(state::ROUTER->use_mpi()) {
      message_set *ms;
      
      while((ms = state::ROUTER->recv_attempt(0)) != NULL) {
         assert(!ms->empty());
         
         for(list_messages::const_iterator it(ms->begin()); it != ms->end(); ++it) {
            message *msg(*it);
            
            new_work(state::DATABASE->find_node(msg->id), msg->data);
            
            delete msg;
         }
         
#ifdef DEBUG_REMOTE
         cout << "Received " << ms->size() << " works" << endl;
#endif
         
         delete ms;
      }
      
      assert(ms == NULL);
   }
}

bool
mpi_static::busy_wait(void)
{
   static const size_t COUNT_UP_TO(2);
    
   size_t cont(0);
   bool turned_inactive(false);
   
   msg_buf.transmit();
   update_pending_messages();
   fetch_work();
   
   while(queue_work.empty()) {

      update_pending_messages();

      if(cont >= COUNT_UP_TO
            && !turned_inactive
				&& msg_buf.all_received())
		{
         turned_inactive = true;
      } else if(cont < COUNT_UP_TO)
         cont++;

      if(turned_inactive)
         state::ROUTER->update_status(router::REMOTE_IDLE);
         
      update_remotes();
      state::ROUTER->update_sent_states();
         
      if(turned_inactive
         && state::ROUTER->all_states_sent()
         && state::ROUTER->finished())
      {
#ifdef DEBUG_REMOTE
         cout << "ITERATION ENDED for " << remote::self->get_rank() << endl;
#endif
         return false;
      }
      
      fetch_work();
   }
   
   assert(!queue_work.empty());
   
   return true;
}

void
mpi_static::work_found(void)
{
   state::ROUTER->update_status(router::REMOTE_ACTIVE);
}

void
mpi_static::update_remotes(void)
{
   if(state::ROUTER->use_mpi())
      state::ROUTER->fetch_updates();
}

void
mpi_static::update_pending_messages(void)
{
   msg_buf.update_received();
   state::ROUTER->update_sent_states();
}

bool
mpi_static::terminate_iteration(void)
{
   generate_aggs();
   
   ++iteration;
   
   if(!queue_work.empty())
      state::ROUTER->send_status(router::REMOTE_ACTIVE);
   else
      state::ROUTER->send_status(router::REMOTE_IDLE);
   
   state::ROUTER->fetch_new_states();
   
   return !state::ROUTER->finished();
}
   
void
mpi_static::new_work_other(sched::base *scheduler, node *node, const simple_tuple *stuple)
{
   assert(0);
}

void
mpi_static::new_work_remote(remote *rem, const process_id proc, message *msg)
{
   msg_buf.insert(rem, proc, msg);
}

mpi_static::mpi_static(void):
   sstatic(0),
   round_trip_fetch(0),
   round_trip_update(0),
   round_trip_send(0)
{
}

mpi_static::~mpi_static(void)
{
}

mpi_static*
mpi_static::start(void)
{
   return new mpi_static();
}

#endif

}