
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
process::do_agg_tuple_add(node *node, vm::tuple *tuple, const ref_count count)
{
   const predicate *pred(tuple->get_predicate()); // get predicate here since tuple can be deleted!
   agg_configuration *conf(node->add_agg_tuple(tuple, count));
   
   if(pred->has_agg_term_info()) {
      const vector<const predicate*>& local_deps(pred->get_local_agg_deps());
      const vector<const predicate*>& remote_deps(pred->get_remote_agg_deps());
      
      if(!local_deps.empty() && remote_deps.empty()) {
         for(size_t i(0); i < local_deps.size(); ++i) {
            const predicate *pred(local_deps[i]);
            //cout << "Checking predicate " << local_deps[i]->get_name() << endl;
            if(!node->no_more_to_process(pred->get_id())) {
               //printf("FAILED!\n");
               return;
            }
         }
      }
      
      if(!remote_deps.empty()) {
         const predicate *remote_pred(remote_deps[0]);
         
         if(!node->no_more_to_process(remote_pred->get_id()))
            return;
         
         const size_t total_remote(node->count_total(remote_pred->get_id()));
         const size_t total_agg(conf->size());
            
         // cout << node->get_id() << " Total remote: " << total_remote << " TOTAL AGG: " << total_agg << endl;
         
         if(!(total_agg == total_remote+1))
            return;
      }
         
         
      simple_tuple_list list;
      conf->generate(pred->get_aggregate_type(), pred->get_aggregate_field(), list);
         
      for(simple_tuple_list::iterator it(list.begin()); it != list.end(); ++it) {
         simple_tuple *tpl(*it);
            
         //cout << node->get_id() << " AUTO GENERATING " << *tpl << endl;
         scheduler->new_work_agg(node, tpl);
      }
   }
}

void
process::do_work(node *node, const simple_tuple *_stuple, const bool ignore_agg)
{
   auto_ptr<const simple_tuple> stuple(_stuple);
   vm::tuple *tuple = stuple->get_tuple();
   ref_count count = stuple->get_count();
   
   // cout << node->get_id() << " " << *stuple << " " << ignore_agg << endl;
   
   ++total_processed;
   
   if(count == 0)
      return;
      
   node->less_to_process(tuple->get_predicate_id());
   
   if(count > 0) {
      if(tuple->is_aggregate() && !ignore_agg)
         do_agg_tuple_add(node, tuple, count);
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
      
      // cout << id << " -------- END ITERATION ---------" << endl;
      
      // false from end_iteration ends program
      if(!scheduler->end_iteration())
         return;
   }
}

void
process::loop(void)
{
   // start process pool
   mem::create_pool(id);
   
   scheduler->init(state::NUM_THREADS);
      
   do_loop();
   
   scheduler->assert_end();
   scheduler->end();
   // cout << "DONE " << id << endl;
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
