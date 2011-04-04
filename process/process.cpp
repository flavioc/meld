
#include <iostream>
#include <boost/function.hpp>

#include "process/process.hpp"
#include "vm/exec.hpp"

using namespace db;
using namespace vm;
using namespace std;
using namespace utils;
using namespace boost;

namespace process {
   
program *process::PROGRAM = NULL;
database *process::DATABASE = NULL;

void
process::enqueue_work(node* node, simple_tuple* stpl)
{
   mutex::scoped_lock lock(mutex);
   
   if(queue.empty() && process_state == PROCESS_INACTIVE) {
      process_state = PROCESS_ACTIVE;
      state::MACHINE->process_is_active();
   }
   
   queue.push_back(pair_node_tuple(node, stpl));
   
   condition.notify_one();
}

void
process::add_node(node *node)
{
   nodes.push_back(node);
   if(nodes_interval == NULL)
      nodes_interval = new interval<node_id>(node->get_id(), node->get_id());
   else
      nodes_interval->update(node->get_id());
}

void
process::do_work(node *node, simple_tuple *stuple)
{
   vm::tuple *tuple = stuple->get_tuple();
   ref_count count = stuple->get_count();

   //cout << "Process " << *stuple << endl;
   delete stuple;
      
   if(count == 0)
      return;
         
   if(count > 0) {
      const bool is_new(node->add_tuple(tuple, count));
         
      if(is_new) {
         // set vm state
         state.tuple = tuple;
         state.node = node;
         state.count = count;
         execute_bytecode(state.PROGRAM->get_bytecode(tuple->get_predicate_id()), state);
      } else {
         delete tuple;
      }
   } else {
      const node::delete_info result(node->delete_tuple(tuple, -1*count));
         
      if(result.to_delete) { // to be removed
         state.tuple = tuple;
         state.node = node;
         state.count = count;
         execute_bytecode(state.PROGRAM->get_bytecode(tuple->get_predicate_id()), state);
         cout << "COMMITING DELETE\n";
         node->commit_delete(result);
         cout << "NODE: " << *node << endl;
      } else
         delete tuple; // as in the positive case, nothing to do
   }
}

process::pair_node_tuple
process::get_work(void)
{
   mutex::scoped_lock lock(mutex);
   
   while(queue.empty()) {
      if(process_state == PROCESS_ACTIVE) {
         process_state = PROCESS_INACTIVE;
         state::MACHINE->process_is_inactive();
      }
      condition.wait(lock);
      if(process_state == PROCESS_INACTIVE) {
         state::MACHINE->process_is_active();
         process_state = PROCESS_ACTIVE;
      }
   }
   
   pair_node_tuple work(queue.front());
   
   queue.pop_front();
   
   return work;
}
   
void
process::loop(void)
{
   predicate *init_pred(state.PROGRAM->get_init_predicate());
   
   for(list_nodes::iterator it(nodes.begin());
      it != nodes.end(); ++it)
   {
      node *cur_node(*it);
      
      enqueue_work(cur_node, simple_tuple::create_new(new vm::tuple(init_pred)));
   }
   
   while(true) {
      pair_node_tuple p(get_work());
      
      do_work(p.first, p.second);
   }
   // NEVER REACHED
}

void
process::start(void)
{
   if(get_id() == 0) {
      thread = new boost::thread();
      loop();
   } else {
      auto fun = [this] (void) -> void { this->loop(); };
      thread = new boost::thread(fun);//bind(&process::loop, this));
   }
}

process::process(const process_id& _id):
   nodes_interval(NULL), thread(NULL), id(_id),
   process_state(PROCESS_ACTIVE)
{
}

void
process::print(ostream& cout) const
{
   cout << *nodes_interval;
}

ostream& operator<<(ostream& cout, const process& proc)
{
   proc.print(cout);
   return cout;
}

}