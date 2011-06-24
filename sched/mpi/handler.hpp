
#ifndef SCHED_MPI_HANDLER_HPP
#define SCHED_MPI_HANDLER_HPP

#include <boost/thread/mutex.hpp>

#include "conf.hpp"

#include "sched/mpi/message_buffer.hpp"
#include "sched/mpi/tokenizer.hpp"

namespace sched
{
   
class mpi_handler
{
private:
   
   size_t step_fetch;
   size_t step_send;
   size_t round_trip_fetch;
   size_t round_trip_update;
   size_t round_trip_send;
   
   message_buffer msg_buf;
   
protected:
   
   static volatile bool iteration_finished;
   static sched::tokenizer token;
   static boost::mutex tok_mutex;
   
   virtual void new_mpi_message(db::node *, db::simple_tuple *) = 0;
   
   virtual void messages_were_transmitted(const size_t total)
   {  
      boost::mutex::scoped_lock lock(tok_mutex);
      token.messages_transmitted(total);
   }
   
   virtual void messages_were_received(const size_t total)
   {
      boost::mutex::scoped_lock lock(tok_mutex);
      token.messages_received(total);
   }
   
   inline void buffer_message(process::remote *rem, const db::node::node_id id, message *msg)
   {
      if(msg_buf.insert(rem, id, msg))
         messages_were_transmitted(1);
   }
   
public:
   
   static void init(void);
   
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
