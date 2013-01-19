
#ifndef PROCESS_REMOTE_HPP
#define PROCESS_REMOTE_HPP

#include "conf.hpp"

#include <ostream>
#ifdef COMPILE_MPI
#include <boost/mpi.hpp>
#endif
#include <stdexcept>
#include <vector>

#include "db/database.hpp"
#include "db/node.hpp"

namespace process
{
   
class remote
{
public:
   
   typedef int remote_id;
   
   static const remote_id LEADER_RANK = 0;
   static remote *self;
   static size_t world_size;
   
private:
   
   const remote_id addr;
   const size_t num_threads;
   
   bool last_one;
   size_t total_nodes;
   size_t nodes_base;
   size_t nodes_per_proc;
   
public:
   
   void cache_values(const size_t, const size_t, const size_t);
   
   inline bool am_last_one(void) const
   {
      return last_one;
   }
   
   static inline bool i_am_last_one(void)
   {
      return self->am_last_one();
   }
   
   static inline std::ostream& rout(std::ostream& out)
   {
      out << self->get_rank() << ": ";
      return out;
   }
   
   inline size_t get_total_nodes(void) const
   {
      return total_nodes;
   }
   
   inline db::node::node_id get_nodes_base(void) const
   {
      return nodes_base;
   }
   
   inline size_t get_nodes_per_proc(void) const
   {
      return nodes_per_proc;
   }
   
   inline vm::process_id find_proc_owner(const db::node::node_id id) const
   {
      assert(get_nodes_per_proc() != 0);
      
      const db::node::node_id remote_node_id(id - get_nodes_base());
      return std::min(remote_node_id / get_nodes_per_proc(), get_num_threads()-1);
   }
   
   inline db::node::node_id find_first_node(const vm::process_id id) const
   {
      return get_nodes_base() + id * nodes_per_proc;
   }
   
   inline db::node::node_id find_last_node(const vm::process_id id) const
   {
      if(num_threads - 1 == id)
         return get_nodes_base() + get_total_nodes();
      else
         return find_first_node(id + 1);
   }
   
   inline remote_id get_rank(void) const { return addr; }
   inline bool is_leader(void) const { return get_rank() == LEADER_RANK; }
   inline bool is_last(void) const { return get_rank() == (remote_id)(world_size-1); }
   
   inline remote_id left_remote_id(void) const
   {
      if(get_rank() == 0)
         return (remote_id)(world_size - 1);
      else
         return (remote_id)(get_rank() - 1);
   }
   
   inline remote_id right_remote_id(void) const
   {
      if(is_last())
         return 0;
      else
         return get_rank() + 1;
   }
   
   inline size_t get_num_threads(void) const { return num_threads; }
   
   explicit remote(const remote_id _addr, const size_t _num_threads):
      addr(_addr), num_threads(_num_threads)
   {
      assert(num_threads != 0);
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
