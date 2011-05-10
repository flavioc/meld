
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
#include "sched/static_global.hpp"
#include "sched/mpi.hpp"
#include "sched/static_local.hpp"
#include "sched/dynamic_local.hpp"
#include "sched/mpi_thread.hpp"
#include "process/sched.hpp"

namespace process
{

// forward declaration   
class process;

class machine
{
private:
   
   const std::string filename;
   const size_t num_threads;
   const scheduler_type sched_type;
   bool will_show_database;
   bool will_dump_database;
   
   std::vector<process*> process_list;
   
   router& rout;
   
public:
   
   void show_database(void) { will_show_database = true; }
   void dump_database(void) { will_dump_database = true; }
   
   process *get_process(const vm::process_id id) { return process_list[id]; }
   
   bool same_place(const db::node::node_id, const db::node::node_id) const;
   
   inline void route_self(process *proc, db::node *node, const db::simple_tuple *stpl)
   {
      proc->get_scheduler()->new_work(node, node, stpl);
   }
   
   void route(process *, const db::node::node_id, const db::simple_tuple*);
   
   void start(void);
   
   explicit machine(const std::string&, router&, const size_t, const scheduler_type);
               
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

