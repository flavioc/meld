
#ifndef THREAD_MPI_SINGLE_HPP
#define THREAD_MPI_SINGLE_HPP

#include "thread/single.hpp"
#include "sched/mpi/tokenizer.hpp"
#include "sched/mpi/handler.hpp"
#include "utils/macros.hpp"
#include "sched/thread/assert.hpp"

namespace sched
{

class mpi_thread_single: public sched::threads_single,
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
   
   static std::vector<sched::base*>& start(const size_t);
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_node(id, trans);
   }
   
   explicit mpi_thread_single(const vm::process_id id):
      threads_single(id)
   {
   }
   
   virtual ~mpi_thread_single(void)
   {
   }
};

}

#endif
