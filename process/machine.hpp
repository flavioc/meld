
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
#include "sched/threads.hpp"
#include "sched/mpi.hpp"

namespace process
{
   
enum scheduler_type {
   SCHED_UNKNOWN,
   SCHED_THREADS_STATIC,
   SCHED_MPI_UNI_STATIC
};

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
   
   void distribute_nodes(db::database *, std::vector<sched::sstatic*>&);
   
public:
   
   void show_database(void) { will_show_database = true; }
   void dump_database(void) { will_dump_database = true; }
   
   process *get_process(const vm::process_id id) { return process_list[id]; }
   
   bool same_place(const db::node::node_id, const db::node::node_id) const;
   
   inline void route_self(process *proc, db::node *node, const db::simple_tuple *stpl)
   {
      proc->get_scheduler()->new_work(node, stpl);
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

