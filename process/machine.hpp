
#ifndef PROCESS_MACHINE_HPP
#define PROCESS_MACHINE_HPP

#include "conf.hpp"

#include <stdexcept>
#include <string>
#include <vector>
#include <boost/thread/mutex.hpp>
#include <boost/thread/barrier.hpp>
#ifdef COMPILE_MPI
#include <boost/mpi.hpp>
#endif

#include "db/database.hpp"
#include "process/router.hpp"
#include "db/tuple.hpp"
#include "db/node.hpp"
#include "sched/local/threads_static.hpp"
#include "sched/local/threads_static_prio.hpp"
#include "sched/local/threads_single.hpp"
#include "sched/local/threads_dynamic.hpp"
#include "sched/local/threads_direct.hpp"
#include "sched/local/mpi_threads_static.hpp"
#include "sched/local/mpi_threads_single.hpp"
#include "sched/types.hpp"
#include "stat/slice_set.hpp"
#include "vm/state.hpp"

namespace process
{

// forward declaration   
class process;

class machine
{
private:
   
   const std::string filename;
   const size_t num_threads;
   const sched::scheduler_type sched_type;
   
   std::vector<process*> process_list;
   
   router& rout;
   
   boost::thread *alarm_thread;
   statistics::slice_set slices;
   
	void execute_const_code(void);
   void deactivate_signals(void);
   void slice_function(void);
   void set_timer(void);
   
public:
   
   sched::scheduler_type get_sched_type(void) const { return sched_type; }
   
   process *get_process(const vm::process_id id) { return process_list[id]; }
   
   bool same_place(const db::node::node_id, const db::node::node_id) const;
   
   void route_self(process *, db::node *, db::simple_tuple *);
   
   void route(const db::node *, process *, const db::node::node_id, db::simple_tuple*);
   
   void start(void);
   
   explicit machine(const std::string&, router&, const size_t, const sched::scheduler_type, const vm::machine_arguments&);
               
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

