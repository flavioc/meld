
#ifndef SCHED_MPI_THREAD_HPP
#define SCHED_MPI_THREAD_HPP

#include "sched/dynamic_local.hpp"
#include "sched/tokenizer.hpp"
#include "sched/buffer.hpp"

namespace sched
{

class mpi_thread: public sched::dynamic_local
{
private:
   
   utils::byte _pad_mpi_thread1[128];
   
   size_t round_trip_update;
   size_t round_trip_send;
   
   utils::byte _pad_mpi_thread2[128];
   
   buffer msg_buf;
   
   void fetch_work(void);
   
   virtual void change_node(thread_node *, dynamic_local *);
   void update_pending_messages(void);
   void transmit_messages(void);
   virtual bool busy_wait(void);
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   virtual bool terminate_iteration(void);
   
public:
   
   inline const bool leader_thread(void) const { return get_id() == 0; }
   
   virtual bool get_work(work_unit&);
   virtual void new_work_remote(process::remote *, const db::node::node_id, process::message *);
   
   static std::vector<sched::base*>& start(const size_t);
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_node(id, trans);
   }
   
   explicit mpi_thread(const vm::process_id id):
      dynamic_local(id),
      round_trip_update(0),
      round_trip_send(0)
   {
   }
   
   virtual ~mpi_thread(void)
   {
   }
};

}

#endif
