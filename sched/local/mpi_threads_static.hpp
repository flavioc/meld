
#ifndef SCHED_LOCAL_MPI_THREADS_STATIC_HPP
#define SCHED_LOCAL_MPI_THREADS_STATIC_HPP

#include "sched/local/threads_static.hpp"
#include "sched/mpi/tokenizer.hpp"
#include "sched/mpi/handler.hpp"

namespace sched
{

class mpi_thread_static: public sched::static_local,
                  private sched::mpi_handler
{
private:
   
   utils::byte _pad_mpi_thread1[128];
   
   message_buffer msg_buf;
   
   void new_mpi_message(db::node *, db::simple_tuple *);
   
   virtual bool busy_wait(void);
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   virtual bool terminate_iteration(void);
   
public:
   
   virtual bool get_work(work_unit&);
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
   
   static std::vector<sched::base*>& start(const size_t);
   
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
