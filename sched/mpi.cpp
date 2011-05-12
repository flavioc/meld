
#include "sched/mpi.hpp"

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
      try_fetch_token_as_worker_if_global();
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
            
            new_work(NULL, state::DATABASE->find_node(msg->id), msg->data);
            
            delete msg;
         }
         
#ifdef DEBUG_REMOTE
         cout << "Received " << ms->size() << " works" << endl;
#endif

         one_message_received();
         
         delete ms;
      }
      
      assert(ms == NULL);
   }
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
      
      if(!busy_loop_token(turned_inactive))
         return false;
      
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
   token_terminate_iteration();
   
   generate_aggs();
   
   ++iteration;
   
   const bool ret(state::ROUTER->reduce_continue(!queue_work.empty()));
   
   if(ret)
      token_is_not_over();
   else
      token_is_over();
   
   return ret;
}
   
void
mpi_static::new_work_other(sched::base *scheduler, node *node, const simple_tuple *stuple)
{
   assert(0);
}

void
mpi_static::new_work_remote(remote *rem, const node::node_id, message *msg)
{
   // this is buffered as the 0 thread id, since only one thread per proc exists
   if(msg_buf.insert(rem, 0, msg))
      tok.transmitted();
}

mpi_static*
mpi_static::find_scheduler(const node::node_id)
{
   return this;
}

mpi_static::mpi_static(void):
   sstatic(0),
   round_trip_fetch(0),
   round_trip_update(0),
   round_trip_send(0),
   round_trip_token(0)
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