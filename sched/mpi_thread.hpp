
#ifndef SCHED_MPI_THREAD_HPP
#define SCHED_MPI_THREAD_HPP

#include "sched/dynamic_local.hpp"
#include "sched/tokenizer.hpp"

namespace sched
{

class mpi_thread: public sched::dynamic_local,
                  private sched::tokenizer
{
private:
   
   
public:
   
   static std::vector<sched::base*>& start(const size_t);
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_node(id, trans);
   }
   
   explicit mpi_thread(const vm::process_id id):
      dynamic_local(id)
   {
   }
   
   virtual ~mpi_thread(void)
   {
   }
};

}

#endif
