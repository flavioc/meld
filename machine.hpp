
#ifndef PROCESS_MACHINE_HPP
#define PROCESS_MACHINE_HPP

#include <stdexcept>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include "db/database.hpp"
#include "db/node.hpp"
#include "stat/slice_set.hpp"
#include "vm/state.hpp"
#include "thread/threads.hpp"

namespace process
{

class machine
{
private:
   
   vm::all *all;
   const std::string filename;

   size_t nodes_per_thread;
   
#ifdef INSTRUMENTATION
   std::thread *alarm_thread;
   statistics::slice_set slices;
#endif

	void execute_const_code(void);
   void deactivate_signals(void);
   void slice_function(void);
   void set_timer(void);

   inline size_t total_nodes(void) const
   {
      return all->DATABASE->static_max_id() + 1;
   }
   
public:

   db::node::node_id find_first_node(const vm::process_id id) const
   {
      return std::min(total_nodes(), id * nodes_per_thread);
   }

   db::node::node_id find_last_node(const vm::process_id id) const
   {
      return find_first_node(id + 1);
   }

   inline size_t find_owned_nodes(const vm::process_id id) const
   {
      return find_last_node(id) - find_first_node(id);
   }
   
   vm::all *get_all(void) const { return this->all; }
   
   void run_action(sched::threads_sched *, vm::tuple *, vm::predicate *,
         vm::candidate_gc_nodes&);
   
   void start(void);
   void init_sched(const vm::process_id);
   
   explicit machine(const std::string&, const size_t,
         const vm::machine_arguments& args = vm::machine_arguments(),
         const std::string& data_file = std::string());
               
   ~machine(void);
};

class machine_error : public std::runtime_error {
 public:
    explicit machine_error(const std::string& msg) :
         std::runtime_error(msg)
    {}
};

}

#endif

