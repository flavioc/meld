
#ifndef THREAD_MPI_STATIC_HPP
#define THREAD_MPI_STATIC_HPP

#include "thread/static.hpp"
#include "sched/mpi/tokenizer.hpp"
#include "sched/mpi/handler.hpp"
#include "utils/macros.hpp"
#include "sched/thread/assert.hpp"

namespace sched
{

class mpi_thread_static: public sched::static_local,
                  private sched::mpi_handler
{
private:
   
   void new_mpi_message(db::node *, db::simple_tuple *);
   
   virtual bool busy_wait(void);
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   virtual bool terminate_iteration(void);
   
public:
   
   virtual bool get_work(process::work&);
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
   
   DEFINE_START_FUNCTION(mpi_thread_static)
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_node(id, trans);
   }
   
   explicit mpi_thread_static(const vm::process_id id):
      static_local(id)
   {
   }
   
   virtual ~mpi_thread_static(void)
   {
   }
};

}

#endif
