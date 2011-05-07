
#include <iostream>
#include <boost/function.hpp>
#include <boost/thread/tss.hpp>

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

static thread_specific_ptr<process> proc_ptr(NULL);

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
   
   //cout << node->get_id() << " " << *stuple << endl;
   
   ++total_processed;
   
   if(count == 0)
      return;
   
   if(count > 0) {
      if(tuple->is_aggregate() && !ignore_agg)
         node->add_agg_tuple(tuple, count);
      else
         do_tuple_add(node, tuple, count);
   } else {
      count = -count;
      
      if(tuple->is_aggregate() && !ignore_agg) {
         node->remove_agg_tuple(tuple, count);
      } else {
         node::delete_info deleter(node->delete_tuple(tuple, count));
         
         if(deleter.to_delete()) { // to be removed
            state.tuple = tuple;
            state.node = node;
            state.count = -count;
            execute_bytecode(state.PROGRAM->get_bytecode(tuple->get_predicate_id()), state);
            deleter();
         } else
            delete tuple; // as in the positive case, nothing to do
      }
   }
}

void
process::do_loop(void)
{
   sched::work_unit work;

   while(true) {
      while(scheduler->get_work(work)) {
         do_work(work.work_node, work.work_tpl, work.agg);
         scheduler->finish_work(work);
      }
   
      scheduler->assert_end_iteration();
      
      if(!scheduler->terminate_iteration())
         return;
   }
}

void
process::loop(void)
{
   // start process pool
   mem::create_pool(id);
   proc_ptr.reset(this);
   
   scheduler->init(state::NUM_THREADS);
      
   do_loop();
   
   scheduler->assert_end();
   scheduler->end();
}

void
process::start(void)
{
   if(id == 0) {
      thread = new boost::thread();
      loop();
   } else
      thread = new boost::thread(bind(&process::loop, this));
}

process::process(const process_id _id, sched::base *_sched):
   id(_id),
   thread(NULL),
   scheduler(_sched),
   state(this),
   total_processed(0)
{
}

process::~process(void)
{
   delete thread;
   delete scheduler;
}

}
