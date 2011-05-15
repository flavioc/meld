
#ifndef SCHED_MPI_HANDLER_HPP
#define SCHED_MPI_HANDLER_HPP

#include "conf.hpp"

#include "sched/mpi/message_buffer.hpp"
#include "sched/mpi/tokenizer.hpp"

namespace sched
{
   
class mpi_handler
{
private:
   
   size_t step_fetch;
   size_t round_trip_fetch;
   size_t round_trip_update;
   size_t round_trip_send;
   
   message_buffer msg_buf;
   
protected:
   
   virtual void new_mpi_message(message *) = 0;
   virtual void messages_were_transmitted(const size_t) = 0;
   virtual void messages_were_received(const size_t) = 0;
   
   inline void buffer_message(process::remote *rem, const db::node::node_id id, message *msg)
   {
      if(msg_buf.insert(rem, id, msg))
         messages_were_transmitted(1);
   }
   
public:
   
   void fetch_work(void);
   void assert_mpi(void) const { assert(msg_buf.empty()); }
   
   void transmit_messages(void);
   
   void update_pending_messages(const bool);
   
   void do_mpi_worker_cycle(void);
   void do_mpi_leader_cycle(void);
   
   explicit mpi_handler(void);
   
   ~mpi_handler(void) {}
};
   
}

#endif
