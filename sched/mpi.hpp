
#ifndef SCHED_MPI_HPP
#define SCHED_MPI_HPP

#include "sched/static.hpp"
#include "sched/buffer.hpp"
#include "sched/token.hpp"

namespace sched
{

#ifdef COMPILE_MPI

class mpi_static: public sched::sstatic
{
private:
   
   size_t round_trip_fetch;
   size_t round_trip_update;
   size_t round_trip_send;
   size_t round_trip_token;
   
   buffer msg_buf;
   token tok;
   bool has_global_tok;
   token global_tok;
   
   void update_pending_messages(void);
   void fetch_work(void);
   void transmit_messages(void);
   void try_fetch_token_as_worker(void);
   void send_token_as_leader(void);
   void send_token_as_idler(void);
   bool try_fetch_token_as_leader(void);
   bool try_fetch_token_as_idler(void);
   bool try_fetch_end_iteration(void);
   void do_collective_end_iteration(size_t);
   virtual void work_found(void);
   virtual bool busy_wait(void);
   virtual bool terminate_iteration(void);
   virtual void assert_end_iteration(void);
   virtual void assert_end(void);
   
public:
   
   virtual void begin_get_work(void);
   
   virtual void new_work_other(sched::base *, db::node *, const db::simple_tuple *);
   virtual void new_work_remote(process::remote *, const vm::process_id, process::message *);
   
   mpi_static *find_scheduler(const db::node::node_id);
   
   static mpi_static *start(void);
   
   explicit mpi_static(void);
   
   virtual ~mpi_static(void);
};

#endif

}

#endif
