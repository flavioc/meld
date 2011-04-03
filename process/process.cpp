
#include <iostream>

#include "process/process.hpp"
#include "vm/exec.hpp"

using namespace db;
using namespace vm;
using namespace std;

namespace process {
   
program *process::PROGRAM = NULL;
database *process::DATABASE = NULL;

void
process::do_node(node *node)
{
   while(!node->queue_empty()) {
      simple_tuple *stuple(node->dequeue_tuple());
      tuple *tuple = stuple->get_tuple();
      ref_count count = stuple->get_count();
      
      cout << "Process " << *stuple << endl;
      delete stuple;
      
      if(count == 0)
         continue; // in principle, this should not happen
         
      if(count > 0) {
         const bool is_new(node->add_tuple(tuple, count));
         
         if(is_new) {
            // set vm state
            state.tuple = tuple;
            state.node = node;
            state.count = count;
            execute_bytecode(state.prog->get_bytecode(tuple->get_predicate_id()), state);
         } else {
            delete tuple;
         }
      } else {
         const node::delete_info result(node->delete_tuple(tuple, -1*count));
         
         if(result.to_delete) { // to be removed
            state.tuple = tuple;
            state.node = node;
            state.count = count;
            execute_bytecode(state.prog->get_bytecode(tuple->get_predicate_id()), state);
            cout << "COMMITING DELETE\n";
            node->commit_delete(result);
            cout << "NODE: " << *node << endl;
         } else
            delete tuple; // as in the positive case, nothing to do
      }
   }
}
   
void
process::loop(void)
{
   bool all_done = false;
   
   while(!all_done) {
      for(list_nodes::iterator it(nodes.begin());
         it != nodes.end(); ++it)
      {
         do_node(*it);
      }
      
      all_done = true;
      
      for(list_nodes::iterator it(nodes.begin());
         it != nodes.end(); ++it)
      {
         node* node(*it);
         
         if(!node->queue_empty()) {
            all_done = false;
            break;
         }
      }
      
      if(all_done)
         break;
   }
   
   for(list_nodes::iterator it(nodes.begin());
      it != nodes.end(); ++it)
   {
      node* node(*it);
      
      node->print(cout);
   }
}

void
process::start(void)
{
   state.prog = process::PROGRAM;
   state.db = process::DATABASE;
   
   state.prog->print_bytecode(cout);
   
   predicate *init_pred(state.prog->get_init_predicate());
   
   for(list_nodes::iterator it(nodes.begin());
      it != nodes.end(); ++it)
   {
      node *cur_node(*it);
      cur_node->enqueue_tuple(simple_tuple::create_new(new tuple(init_pred)));
   }
   
   loop();
   
#if 0
   for(list_nodes::iterator it(nodes.begin());
      it != nodes.end(); ++it)
   {
      node *cur_node(*it);
      tuple *new_tuple(new tuple(state.prog->get_predicate_by_name("another")));
      
      new_tuple->set_float(0, 1.0);
      simple_tuple *stuple(simple_tuple::remove_new(new_tuple));
      
      cout << *stuple << endl;
      cur_node->enqueue_tuple(stuple);
   }
   
   cout << "REMOVE PHASE>>>>>>>>>>>>>>>>\n";
   loop();
#endif
}

}