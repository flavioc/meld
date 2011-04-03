
#ifndef PROCESS_EXEC_HPP
#define PROCESS_EXEC_HPP

#include <string>
#include <vector>
#include <boost/thread/mutex.hpp>

#include "db/database.hpp"

namespace process
{
   
   class process;

class machine
{
private:
   
   const std::string filename;
   const size_t num_threads;
   size_t threads_active;
   
   boost::mutex active_mutex;
   
   std::vector<process*> process_list;
   
   void distribute_nodes(db::database *);
   
public:
  
   process* find_owner(const db::node*) const;
   
   void process_is_active(void);
   
   void process_is_inactive(void);
   
   void start(void);
   
   explicit machine(const std::string& file, const size_t th);
};

}

#endif

