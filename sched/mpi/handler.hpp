
#ifndef SCHED_MPI_HANDLER_HPP
#define SCHED_MPI_HANDLER_HPP

#include <boost/thread/mutex.hpp>

#include "conf.hpp"

#include "sched/mpi/message_buffer.hpp"
#include "sched/mpi/tokenizer.hpp"
#include "utils/atomic.hpp"

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
   
   utils::byte recv_buf[MPI_BUF_SIZE];
   
   static utils::atomic<size_t> inside_counter;
   
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
   
   bool attempt_token(boost::function0<bool>&, const bool);
   
public:
   
   static void init(const size_t);
   static void end(void);
   
   void fetch_work(const vm::process_id);
   
   void assert_mpi(void) const
   {
      assert(msg_buf.empty());
      assert(iteration_finished);
   }
   
   void transmit_messages(void);
   
   void update_pending_messages(const bool);
   
   void do_mpi_worker_cycle(void);
   void do_mpi_leader_cycle(void);
   void do_mpi_cycle(const vm::process_id);
   
   explicit mpi_handler(void);
   
   ~mpi_handler(void) {}
};

#define BUSY_LOOP_CHECK_INACTIVE_THREADS() \
   if(is_inactive() && !has_work() && leader_thread() && all_threads_finished()) {  \
      mutex::scoped_lock lock(tok_mutex);                                           \
      if(!token.busy_loop_token(all_threads_finished())) {                          \
         assert(all_threads_finished());                                            \
         assert(is_inactive());                                                     \
         assert(!has_work());                                                       \
         iteration_finished = true;                                                 \
         return false;                                                              \
      }                                                                             \
   }
   
#define BUSY_LOOP_CHECK_INACTIVE_MPI() \
   if(!leader_thread() && !has_work() && is_inactive() && all_threads_finished() && iteration_finished) {   \
      assert(!leader_thread());                                                                             \
      assert(is_inactive());                                                                                \
      assert(!has_work());                                                                                  \
      assert(iteration_finished);                                                                           \
      assert(all_threads_finished());                                                                       \
      return false;                                                                                         \
   }
   
#define BUSY_LOOP_FETCH_WORK()   \
   if(leader_thread())           \
      fetch_work(0);
      
#define MPI_WORK_CYCLE()         \
   do_mpi_worker_cycle();        \
   if(leader_thread())           \
      do_mpi_leader_cycle();

#define IDLE_MPI()                  \
   transmit_messages();             \
   if(leader_thread())              \
      fetch_work(0);                \
   update_pending_messages(true);

#define IDLE_MPI_ALL(ID)            \
   transmit_messages();             \
   fetch_work(ID);                  \
   update_pending_messages(true);   \

}

#endif
