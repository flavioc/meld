
#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <list>
#include <stdexcept>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include "vm/program.hpp"
#include "vm/state.hpp"
#include "db/node.hpp"
#include "db/database.hpp"
#include "utils/interval.hpp"

namespace process
{
   
typedef unsigned short process_id;

class process
{
private:
   
   typedef std::list<db::node*> list_nodes;
   typedef std::pair<db::node*, db::simple_tuple*> pair_node_tuple;
   typedef std::list<pair_node_tuple> work_queue;
   
   utils::interval<db::node_id> *nodes_interval;
   boost::thread *thread;
   process_id id;
   
   boost::mutex mutex;
   boost::condition condition;
   list_nodes nodes;
   vm::state state;
   work_queue queue;
   
   enum {
      PROCESS_ACTIVE,
      PROCESS_INACTIVE
   } process_state;
   
   void do_work(db::node *, db::simple_tuple *);
   pair_node_tuple get_work(void);
   void loop(void);

public:
   
   static db::database *DATABASE;
   static vm::program *PROGRAM;
   
   void add_node(db::node*);
   
   void enqueue_work(db::node*, db::simple_tuple*);
   
   const process_id get_id(void) const { return id; }
   
   void start(void);
   
   void notify(void);
   
   bool owns_node(const db::node_id& id) const { return nodes_interval->between(id); }
   
   void print(std::ostream&) const;
   
   explicit process(const process_id&);
};

std::ostream& operator<<(std::ostream&, const process&);

class process_exec_error : public std::runtime_error {
 public:
    explicit process_exec_error(const std::string& msg) :
         std::runtime_error(msg)
    {}
};

}

#endif
