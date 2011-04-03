
#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <list>
#include <boost/thread/thread.hpp>

#include "vm/program.hpp"
#include "vm/state.hpp"
#include "db/node.hpp"
#include "db/database.hpp"

namespace process
{
   
typedef unsigned short process_id;

class process
{
private:
   typedef std::list<db::node*> list_nodes;
   
   list_nodes nodes;
   boost::thread *thread;
   vm::state state;
   process_id id;
   
   void do_node(db::node*);
   
   void loop(void);

public:
   
   static db::database *DATABASE;
   static vm::program *PROGRAM;
   
   void add_node(db::node* node) { nodes.push_back(node); }
   
   process_id get_id(void) const { return id; }
   
   void start(void);
   
   explicit process(const process_id& _id): thread(NULL), id(_id) {}
};

}

#endif
