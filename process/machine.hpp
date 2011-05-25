
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
#include "sched/global/threads_static.hpp"
#include "sched/global/mpi.hpp"
#include "sched/local/threads_static.hpp"
#include "sched/local/threads_single.hpp"
#include "sched/local/threads_dynamic.hpp"
#include "sched/local/mpi_threads_dynamic.hpp"
#include "sched/types.hpp"

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
   bool will_show_memory;
   
   std::vector<process*> process_list;
   
   router& rout;
   
public:
   
   void show_database(void) { will_show_database = true; }
   void dump_database(void) { will_dump_database = true; }
   void show_memory(void) { will_show_memory = true; }
   
   process *get_process(const vm::process_id id) { return process_list[id]; }
   
   bool same_place(const db::node::node_id, const db::node::node_id) const;
   
   void route_self(process *, db::node *, const db::simple_tuple *);
   
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

