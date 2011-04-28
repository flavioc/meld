
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

namespace process
{

// forward declaration   
class process;

class machine
{
private:
   
   const std::string filename;
   const size_t num_threads;
   bool will_show_database;
   bool will_dump_database;
   
   std::vector<process*> process_list;
   
   router& rout;
   
   char _pad1[64];
   
   boost::barrier *proc_barrier;
   
   char _pad2[64];
   
   volatile size_t threads_active;
   
   void distribute_nodes(db::database *);
   
public:
  
   void process_is_active(void);
   
   void process_is_inactive(void);
   
   bool finished(void) const { return threads_active == 0; }
   
   void wait_aggregates(void) { proc_barrier->wait(); }
   
   void show_database(void) { will_show_database = true; }
   void dump_database(void) { will_dump_database = true; }
   
   process *get_process(const vm::process_id id) { return process_list[id]; }
   
   void route(process *, const db::node::node_id, const db::simple_tuple*);
   
   void start(void);
   
   explicit machine(const std::string&, router&, const size_t);
               
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

