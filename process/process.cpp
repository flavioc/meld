
#include <iostream>
#include <boost/function.hpp>

#include "process/process.hpp"
#include "vm/exec.hpp"
#include "process/machine.hpp"
#include "mem/thread.hpp"

using namespace db;
using namespace vm;
using namespace std;
using namespace utils;
using namespace boost;

namespace process {
   
program *process::PROGRAM = NULL;
database *process::DATABASE = NULL;
mutex process::remote_mutex;

void
process::enqueue_work(node* node, const simple_tuple* stpl, const bool is_agg)
{
   work_unit work = {node, stpl, is_agg};
   
   if(process_state == PROCESS_INACTIVE) {
      mutex::scoped_lock lock(mutex);
      if(process_state == PROCESS_INACTIVE) {
         process_state = PROCESS_ACTIVE;
         state::MACHINE->process_is_active();
      }
   }
   
   {
      mutex::scoped_lock lock(mutex);
      queue.push_back(work);
   }
}

bool
process::busy_wait(void)
{  
   while(queue.empty()) {
      
#ifdef COMPILE_MPI
      if(get_id() == 0) {
         update_remotes();
      
         if(state::MACHINE->finished() && state::ROUTER->finished()) {
         
            // wait a bit before making a good decision
            boost::this_thread::sleep(boost::posix_time::millisec(25));
         
            fetch_work();
         
            if(!queue.empty())
               return true;
            
            update_remotes();
            
            if(state::MACHINE->finished() && state::ROUTER->finished()) {
               state::MACHINE->mark_finished();
               printf("ENDING HERE %d\n", remote::self->get_rank());
               return false;
            }
         }
      } else
#endif
 if(state::MACHINE->finished())
         return false;
      
#ifdef COMPILE_MPI   
      fetch_work();
#endif
   }
   
   return true;
}

bool
process::get_work(work_unit& work)
{
#ifdef COMPILE_MPI
   fetch_work();
#endif
   
   if(queue.empty()) {
      mutex.lock();

      if(process_state == PROCESS_ACTIVE) {
         process_state = PROCESS_INACTIVE;
         state::MACHINE->process_is_inactive();
      }
      
      mutex.unlock();
    
      if(!busy_wait())
         return false;
      
      mutex.lock();
         
      if(process_state == PROCESS_INACTIVE) {
         state::MACHINE->process_is_active();
         process_state = PROCESS_ACTIVE;
      }
   } else
      mutex.lock();
   
   work = queue.front();
   
   queue.pop_front();
   
   mutex.unlock();
   
   return true;
}

void
process::add_node(node *node)
{
   nodes.push_back(node);
   
   if(nodes_interval == NULL)
      nodes_interval = new interval<node::node_id>(node->get_id(), node->get_id());
   else
      nodes_interval->update(node->get_id());
}

void
process::fetch_work(void)
{
#ifdef COMPILE_MPI
   if(state::ROUTER->use_mpi()) {
      message *msg;
      
      while((msg = state::ROUTER->recv_attempt(get_id()))) {
         enqueue_work(state::DATABASE->find_node(msg->id), msg->data);
         
         delete msg;
      }
   }
#endif
}

void
process::do_tuple_add(node *node, vm::tuple *tuple, const ref_count count)
{
   const bool is_new(node->add_tuple(tuple, count));

   if(is_new) {
      // set vm state
      state.tuple = tuple;
      state.node = node;
      state.count = count;
      execute_bytecode(state.PROGRAM->get_bytecode(tuple->get_predicate_id()), state);
   } else
      delete tuple;
}

void
process::do_work(node *node, const simple_tuple *_stuple, const bool ignore_agg)
{
   auto_ptr<const simple_tuple> stuple(_stuple);
   vm::tuple *tuple = stuple->get_tuple();
   ref_count count = stuple->get_count();
      
   if(count == 0)
      return;
         
   if(count > 0) {
      if(tuple->is_aggregate() && !ignore_agg)
         node->add_agg_tuple(tuple, count);
      else
         do_tuple_add(node, tuple, count);
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

void
process::update_remotes(void)
{
#ifdef COMPILE_MPI
   if(state::ROUTER->use_mpi() && get_id() == 0) {
      remote_mutex.lock();
      state::ROUTER->fetch_updates();
      remote_mutex.unlock();
   }
#endif
}

void
process::generate_aggs(void)
{
   for(list_nodes::iterator it(nodes.begin());
      it != nodes.end();
      ++it)
   {
      node *no(*it);
      tuple_list ls(no->generate_aggs());

      for(tuple_list::iterator it2(ls.begin());
         it2 != ls.end();
         ++it2)
      {
         enqueue_work(no, simple_tuple::create_new(*it2), true);
      }
   }
}

void
process::do_loop(void)
{
   work_unit work;

   while(true) {
      while(get_work(work))
         do_work(work.work_node, work.work_tpl, work.agg);
   
#ifdef COMPILE_MPI
      if(state::ROUTER->use_mpi()) {
         ended = true;

         if(get_id() == 0) {
            while(!state::MACHINE->all_ended())
               {}
         
            state::ROUTER->finish();
         }
         
         return;
      } else
#endif
      {
         state::MACHINE->wait_aggregates();
         
         generate_aggs();
         
         state::MACHINE->wait_aggregates();

         if(state::MACHINE->finished())
            return;
      }
   }
}

void
process::loop(void)
{
   // start process pool
   mem::create_pool(get_id());
   
   // enqueue init tuple
   predicate *init_pred(state.PROGRAM->get_init_predicate());
   
   for(list_nodes::iterator it(nodes.begin());
      it != nodes.end(); ++it)
   {
      node *cur_node(*it);
      
      enqueue_work(cur_node, simple_tuple::create_new(new vm::tuple(init_pred)));
   }
   
   do_loop();
}

void
process::start(void)
{
   if(get_id() == 0) {
      thread = new boost::thread();
      loop();
   } else
      thread = new boost::thread(bind(&process::loop, this));
}

process::process(const process_id& _id):
   nodes_interval(NULL), thread(NULL), id(_id),
   state(this),
   process_state(PROCESS_ACTIVE),
   ended(false),
   agg_checked(false)
{
}

process::~process(void)
{
   delete nodes_interval;
   delete thread;
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
