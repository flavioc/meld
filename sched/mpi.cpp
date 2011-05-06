
#include "sched/mpi.hpp"

#ifdef COMPILE_MPI

#include <mpi.h>

#include "vm/state.hpp"
#include "process/router.hpp"

using namespace db;
using namespace process;
using namespace vm;
using namespace std;

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
   
   assert(msg_buf.empty());
   assert(msg_buf.all_received());
}

void
mpi_static::assert_end(void)
{
   sstatic::assert_end();
   
   assert(msg_buf.empty());
   assert(msg_buf.all_received());
}

void
mpi_static::transmit_messages(void)
{
   size_t total(msg_buf.transmit());
   
   tok.transmitted(total);
}

void
mpi_static::try_fetch_token_as_worker(void)
{
   assert(has_global_tok == false);
   
   if(state::ROUTER->receive_token(global_tok)) {
      has_global_tok = true;
   }
}
   
void
mpi_static::begin_get_work(void)
{
   static const size_t ROUND_TRIP_FETCH_MPI(40);
   static const size_t ROUND_TRIP_UPDATE_MPI(40);
   static const size_t ROUND_TRIP_SEND_MPI(40);
   static const size_t ROUND_TRIP_TOKEN_MPI(40);
   
   ++round_trip_fetch;
   ++round_trip_update;
   ++round_trip_send;
   ++round_trip_token;
   
   if(round_trip_fetch == ROUND_TRIP_FETCH_MPI) {
      fetch_work();
      round_trip_fetch = 0;
   }
   
   if(round_trip_update == ROUND_TRIP_UPDATE_MPI) {
      update_pending_messages();
      round_trip_update = 0;
   }
   
   if(round_trip_send == ROUND_TRIP_SEND_MPI) {
      transmit_messages();
      round_trip_send = 0;
   }
   
   if(round_trip_token == ROUND_TRIP_TOKEN_MPI) {
      if(!has_global_tok)
         try_fetch_token_as_worker();
      round_trip_token = 0;
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

         tok.received();
         tok.set_black();
         
         delete ms;
      }
      
      assert(ms == NULL);
   }
}

void
mpi_static::send_token_as_leader(void)
{
   assert(remote::self->is_leader());
   assert(has_global_tok == true);
   
#ifdef DEBUG_REMOTE
   cout << "Sent a white token / 0 from leader to" << endl;
#endif

   tok.set_white();
   global_tok.set_white();
   global_tok.reset_count();
   has_global_tok = false;
   
   assert(global_tok.is_white());
   assert(global_tok.is_zero());
   state::ROUTER->send_token(global_tok);
   
   assert(tok.is_white());
}

bool
mpi_static::try_fetch_token_as_leader(void)
{
   assert(remote::self->is_leader());
   assert(has_global_tok == false);
   
   if(state::ROUTER->receive_token(global_tok)) {
      has_global_tok = true;
      global_tok.add_count(tok.get_count());
#ifdef DEBUG_SAFRAS
      cout << "Add count " << tok.get_count() << endl;
#endif
      if(global_tok.is_white() && global_tok.is_zero()) {
#ifdef DEBUG_SAFRAS
         cout << "TERMINATE!" << endl;
#endif
         assert(global_tok.is_white());
         assert(global_tok.is_zero());
         
         state::ROUTER->broadcast_end_iteration(iteration);
         
         return true;
      } else
         send_token_as_leader();
   }
   
   return false;
}

void
mpi_static::send_token_as_idler(void)
{
   assert(!remote::self->is_leader());
   assert(has_global_tok == true);
   //assert(tok.is_black()); // local token must be black here
   
#ifdef DEBUG_SAFRAS
   cout << "Send token as idler in black" << endl;
#endif

   global_tok.add_count(tok.get_count());
   global_tok.set_black();
   
   assert(global_tok.is_black());
   
   state::ROUTER->send_token(global_tok);
   has_global_tok = false;
}

bool
mpi_static::try_fetch_token_as_idler(void)
{
   assert(!remote::self->is_leader());
   assert(has_global_tok == false);
   
   if(state::ROUTER->receive_token(global_tok)) {
      has_global_tok = true;
      global_tok.add_count(tok.get_count());
#ifdef DEBUG_SAFRAS
      cout << "Received new tok and adding count " << tok.get_count() << endl;
#endif

      if(tok.is_black())
         global_tok.set_black();
      
      tok.set_white();
#ifdef DEBUG_SAFRAS
      if(global_tok.is_black())
         cout << "Resending as black\n";
      else
         cout << "Resending as white\n";
#endif

      assert(has_global_tok == true);
      state::ROUTER->send_token(global_tok);
      has_global_tok = false;
      
      assert(tok.is_white());
      
      return true;
   }
   
   assert(has_global_tok == false);
   
   return false;
}

bool
mpi_static::try_fetch_end_iteration(void)
{
   if(state::ROUTER->received_end_iteration())
      return true;
   return false;
}

bool
mpi_static::busy_wait(void)
{
   bool turned_inactive(false);
   
   transmit_messages();
   update_pending_messages();
   fetch_work();
   
   while(queue_work.empty()) {

      update_pending_messages();

      if(!turned_inactive
				&& msg_buf.all_received())
		{
         turned_inactive = true;
      }
         
      if(turned_inactive) {
         if(remote::self->is_leader()) {
            if(has_global_tok && !state::ROUTER->use_mpi())
               return false;
            if(has_global_tok)
               send_token_as_leader();
            else {
               if(try_fetch_token_as_leader())
                  return false;
            }
         } else {
            if(has_global_tok)
               send_token_as_idler();
            else {
               if(!try_fetch_token_as_idler()) {
                  if(try_fetch_end_iteration())
                     return false;
               }
            }
         }
      } else {
         if(!has_global_tok)
            try_fetch_token_as_worker();
      }
      
      fetch_work();
   }
   
   assert(!queue_work.empty());
   
   return true;
}

void
mpi_static::work_found(void)
{
}

void
mpi_static::update_pending_messages(void)
{
   msg_buf.update_received();
}

bool
mpi_static::terminate_iteration(void)
{
   if(remote::self->is_leader()) {
      assert(global_tok.is_zero());
      assert(global_tok.is_white());
      assert(has_global_tok == true);   
   }
   
   assert(tok.is_white());
   
   tok.reset_count();
   
   generate_aggs();
   
   ++iteration;
   
   return state::ROUTER->reduce_continue(!queue_work.empty());
}
   
void
mpi_static::new_work_other(sched::base *scheduler, node *node, const simple_tuple *stuple)
{
   assert(0);
}

void
mpi_static::new_work_remote(remote *rem, const process_id proc, message *msg)
{
   if(msg_buf.insert(rem, proc, msg))
      tok.transmitted();
}

mpi_static::mpi_static(void):
   sstatic(0),
   round_trip_fetch(0),
   round_trip_update(0),
   round_trip_send(0),
   round_trip_token(0)
{
   if(remote::self->is_leader()) {
      has_global_tok = true;
      global_tok.set_white();
   } else {
      has_global_tok = false;
   }
   tok.set_white();
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