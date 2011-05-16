
#ifndef SCHED_GLOBAL_MPI_HPP
#define SCHED_GLOBAL_MPI_HPP

#include "sched/global/static.hpp"
#include "sched/mpi/handler.hpp"
#include "sched/mpi/tokenizer.hpp"

namespace sched
{

#ifdef COMPILE_MPI

class mpi_static: public sched::sstatic,
                  private sched::tokenizer,
                  private sched::mpi_handler
{
private:
   
   void messages_were_transmitted(const size_t);
   void messages_were_received(const size_t);
   void new_mpi_message(db::node *, db::simple_tuple *);
   
   virtual void work_found(void);
   virtual bool busy_wait(void);
   virtual bool terminate_iteration(void);
   virtual void assert_end_iteration(void);
   virtual void assert_end(void);
   
public:
   
   virtual void begin_get_work(void);
   
   virtual void new_work_other(sched::base *, db::node *, const db::simple_tuple *);
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
   
   mpi_static *find_scheduler(const db::node::node_id);
   
   static mpi_static *start(void);
   
   explicit mpi_static(void);
   
   virtual ~mpi_static(void);
};

#endif

}

#endif
