
#ifndef SCHED_GLOBAL_MPI_HPP
#define SCHED_GLOBAL_MPI_HPP

#include "sched/global/threads_static.hpp"
#include "sched/mpi/handler.hpp"
#include "utils/macros.hpp"
#include "sched/thread/assert.hpp"

namespace sched
{

class mpi_static: public sched::static_global,
                  private sched::mpi_handler
{
private:
   
   void new_mpi_message(db::node *, db::simple_tuple *);
   
   virtual bool busy_wait(void);
   virtual bool terminate_iteration(void);
   virtual void assert_end_iteration(void);
   virtual void assert_end(void);
   
public:
   
   virtual bool get_work(process::work&);
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
   
   DEFINE_START_FUNCTION(mpi_static)
   
   explicit mpi_static(const vm::process_id id):
      static_global(id)
   {
   }

   virtual ~mpi_static(void)
   {
   }
};

}

#endif
