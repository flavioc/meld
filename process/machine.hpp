
#ifndef PROCESS_MACHINE_HPP
#define PROCESS_MACHINE_HPP

#include <stdexcept>
#include <string>
#include <vector>
#include <boost/thread/mutex.hpp>
#include <boost/mpi.hpp>
#include <boost/thread/barrier.hpp>

#include "db/database.hpp"
#include "process/router.hpp"
#include "db/tuple.hpp"
#include "db/node.hpp"
#include "conf.hpp"

namespace process
{

// forward declaration   
class process;

class machine
{
private:
   
   const std::string filename;
   const size_t num_threads;
   size_t threads_active;
   
   boost::mutex active_mutex;
   
   std::vector<process*> process_list;
   
   router& rout;
   
   boost::barrier *proc_barrier;
   
   bool is_finished;
   
   bool will_show_database;
   
   void distribute_nodes(db::database *);
   
public:
  
   void process_is_active(void);
   
   void process_is_inactive(void);
   
   bool finished(void) const { return threads_active == 0; }
   
   void wait_aggregates(void) { proc_barrier->wait(); }
   
   bool all_ended(void);
   
   void mark_finished(void) { is_finished = true; }
   
   inline const bool marked_finished(void) { return is_finished; }
   
   void show_database(void) { will_show_database = true; }
   
   void route(const db::node::node_id, const db::simple_tuple*);
   
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

