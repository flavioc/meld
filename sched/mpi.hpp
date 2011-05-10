
#ifndef SCHED_MPI_HPP
#define SCHED_MPI_HPP

#include "sched/static.hpp"
#include "sched/buffer.hpp"
#include "sched/tokenizer.hpp"

namespace sched
{

#ifdef COMPILE_MPI

class mpi_static: public sched::sstatic,
                  private sched::tokenizer
{
private:
   
   size_t round_trip_fetch;
   size_t round_trip_update;
   size_t round_trip_send;
   size_t round_trip_token;
   
   buffer msg_buf;
   
   void update_pending_messages(void);
   void fetch_work(void);
   void transmit_messages(void);
   virtual void work_found(void);
   virtual bool busy_wait(void);
   virtual bool terminate_iteration(void);
   virtual void assert_end_iteration(void);
   virtual void assert_end(void);
   
public:
   
   virtual void begin_get_work(void);
   
   virtual void new_work_other(sched::base *, db::node *, const db::simple_tuple *);
   virtual void new_work_remote(process::remote *, const db::node::node_id, process::message *);
   
   mpi_static *find_scheduler(const db::node::node_id);
   
   static mpi_static *start(void);
   
   explicit mpi_static(void);
   
   virtual ~mpi_static(void);
};

#endif

}

#endif
