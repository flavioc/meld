
#ifndef SCHED_MPI_HPP
#define SCHED_MPI_HPP

#include "sched/static.hpp"
#include "process/buffer.hpp"

namespace sched
{

#ifdef COMPILE_MPI

class mpi_static: public sched::sstatic
{
private:
   
   size_t round_trip_fetch;
   size_t round_trip_update;
   size_t round_trip_send;
   
   process::buffer msg_buf;
   
   void update_pending_messages(void);
   void update_remotes(void);
   void fetch_work(void);
   virtual void work_found(void);
   virtual bool busy_wait(void);
   virtual bool terminate_iteration(void);
   virtual void assert_end_iteration(void);
   virtual void assert_end(void);
   
public:
   
   virtual void begin_get_work(void);
   
   virtual void new_work_other(sched::base *, db::node *, const db::simple_tuple *);
   virtual void new_work_remote(process::remote *, const vm::process_id, process::message *);
   
   static mpi_static *start(void);
   
   explicit mpi_static(void);
   
   virtual ~mpi_static(void);
};

#endif

}

#endif
