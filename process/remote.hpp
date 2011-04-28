
#ifndef PROCESS_REMOTE_HPP
#define PROCESS_REMOTE_HPP

#include "conf.hpp"

#include <ostream>
#ifdef COMPILE_MPI
#include <boost/mpi.hpp>
#endif
#include <stdexcept>
#include <vector>

#include "process/process.hpp"
#include "db/database.hpp"
#include "db/node.hpp"

namespace process
{
   
class remote
{
public:
   
   typedef int remote_id;
   
   static remote *self;
   
private:
   
   const remote_id addr;
   const size_t num_threads;
   
   bool last_one;
   size_t total_nodes;
   size_t nodes_base;
   size_t nodes_per_proc;
   
public:
   
   void cache_values(const size_t, const size_t);
   
   inline const bool am_last_one(void) const
   {
      return last_one;
   }
   
   static inline const bool i_am_last_one(void)
   {
      return self->am_last_one();
   }
   
   static inline std::ostream& rout(std::ostream& out)
   {
      out << self->get_rank() << ": ";
      return out;
   }
   
   inline const size_t get_total_nodes(void) const
   {
      return total_nodes;
   }
   
   inline const db::node::node_id get_nodes_base(void) const
   {
      return nodes_base;
   }
   
   inline const size_t get_nodes_per_proc(void) const
   {
      return nodes_per_proc;
   }
   
   inline const vm::process_id find_proc_owner(const db::node::node_id id) const
   {
      const db::node::node_id remote_node_id(id - get_nodes_base());
      return std::min(remote_node_id / get_nodes_per_proc(), get_num_threads()-1);
   }
   
   inline const remote_id get_rank(void) const { return addr; }
   
   inline const size_t get_num_threads(void) const { return num_threads; }
   
   explicit remote(const remote_id _addr, const size_t _num_threads):
      addr(_addr), num_threads(_num_threads)
   {
   }
};

class remote_error : public std::runtime_error {
 public:
    explicit remote_error(const std::string& msg) :
         std::runtime_error(msg)
    {}
};
   
}

#endif
