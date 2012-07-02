
#ifndef THREAD_MPI_DYNAMIC_HPP
#define THREAD_MPI_DYNAMIC_HPP

#include "thread/dynamic.hpp"
#include "sched/mpi/tokenizer.hpp"
#include "sched/mpi/handler.hpp"
#include "utils/macros.hpp"
#include "sched/thread/assert.hpp"

namespace sched
{

class mpi_thread_dynamic: public sched::dynamic_local,
                          private sched::mpi_handler
{
private:
   
   void new_mpi_message(db::node *, db::simple_tuple *);
   
   virtual void change_node(thread_node *, dynamic_local *);
   virtual bool busy_wait(void);
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   virtual bool terminate_iteration(void);
   
public:
   
   virtual bool get_work(process::work&);
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
   
   DEFINE_START_FUNCTION(mpi_thread_dynamic)
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_node(id, trans);
   }
   
   explicit mpi_thread_dynamic(const vm::process_id id):
      dynamic_local(id)
   {
   }
   
   virtual ~mpi_thread_dynamic(void)
   {
   }
};

}

#endif
