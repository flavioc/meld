
#ifndef PROCESS_MACHINE_HPP
#define PROCESS_MACHINE_HPP

#include <stdexcept>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include "db/database.hpp"
#include "process/router.hpp"
#include "db/node.hpp"
#include "sched/types.hpp"
#include "stat/slice_set.hpp"
#include "vm/state.hpp"
#include "sched/base.hpp"

namespace process
{

class machine
{
private:
   
   vm::all *all;
   const std::string filename;
   const sched::scheduler_type sched_type;
   
   router& rout;
   
   std::thread *alarm_thread;
   statistics::slice_set slices;

	void execute_const_code(void);
   void deactivate_signals(void);
   void slice_function(void);
   void set_timer(void);
   
public:
   
   sched::scheduler_type get_sched_type(void) const { return sched_type; }
   
   vm::all *get_all(void) const { return this->all; }
   
   bool same_place(const db::node::node_id, const db::node::node_id) const;
   
   void run_action(sched::base *, db::node *, vm::tuple *, vm::predicate *
#ifdef GC_NODES
         , vm::candidate_gc_nodes&
#endif
         );
   
   void start(void);
   void init_sched(const vm::process_id);
   
   explicit machine(const std::string&, router&, const size_t, const sched::scheduler_type, const vm::machine_arguments& args = vm::machine_arguments(), const std::string& data_file = std::string());
               
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

