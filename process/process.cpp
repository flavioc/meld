
#include <iostream>
#include <boost/function.hpp>
#include <boost/thread/tss.hpp>
#include <algorithm>

#include "process/process.hpp"
#include "vm/exec.hpp"
#include "process/machine.hpp"
#include "mem/thread.hpp"
#include "db/neighbor_agg_configuration.hpp"

using namespace db;
using namespace vm;
using namespace std;
using namespace utils;
using namespace boost;

namespace process {

static inline byte_code
get_bytecode(vm::tuple *tuple)
{
   return state::PROGRAM->get_predicate_bytecode(tuple->get_predicate_id());
}

void
process::do_tuple_action(node *node, vm::tuple *tuple, const ref_count count)
{
	(void)count;
	state::MACHINE->run_action(scheduler, node, tuple);
	// everything is deleted
}

void
process::do_tuple_add(node *node, vm::tuple *tuple, const ref_count count)
{
   if(tuple->is_linear()) {
      state.setup(tuple, node, count);
      const execution_return ret(execute_bytecode(get_bytecode(tuple), state));
      
      if(ret == EXECUTION_CONSUMED) {
			scheduler->new_linear_consumption(node, tuple);
         delete tuple;
      } else {
			scheduler->new_linear_derivation(node, tuple);
         node->add_tuple(tuple, count);
		}
   } else {
      const bool is_new(node->add_tuple(tuple, count));

		scheduler->new_persistent_derivation(node, tuple);

      if(is_new) {
         // set vm state
         state.setup(tuple, node, count);
         execute_bytecode(get_bytecode(tuple), state);
      } else
         delete tuple;
   }
}

void
process::do_agg_tuple_add(node *node, vm::tuple *tuple, const ref_count count)
{
   const predicate *pred(tuple->get_predicate()); // get predicate here since tuple can be deleted!
   agg_configuration *conf(node->add_agg_tuple(tuple, count));
   const aggregate_safeness safeness(pred->get_agg_safeness());
   
   switch(safeness) {
      case AGG_UNSAFE: return;
      case AGG_IMMEDIATE: {
         simple_tuple_list list;
         
         conf->generate(pred->get_aggregate_type(), pred->get_aggregate_field(), list);

         for(simple_tuple_list::iterator it(list.begin()); it != list.end(); ++it) {
            simple_tuple *tpl(*it);
            scheduler->new_work_agg(node, tpl);
         }
         return;
      }
      break;
      case AGG_LOCALLY_GENERATED: {
         const strat_level level(pred->get_agg_strat_level());
         
         if(node->get_local_strat_level() < level) {
            return;
         }
      }
      break;
      case AGG_NEIGHBORHOOD:
      case AGG_NEIGHBORHOOD_AND_SELF: {
         const neighbor_agg_configuration *neighbor_conf(dynamic_cast<neighbor_agg_configuration*>(conf));
   
         if(!neighbor_conf->all_present()) {
            return;
         }
      }
      break;
      default: return;
   }

   simple_tuple_list list;
   conf->generate(pred->get_aggregate_type(), pred->get_aggregate_field(), list);

   for(simple_tuple_list::iterator it(list.begin()); it != list.end(); ++it) {
      simple_tuple *tpl(*it);
      
      assert(tpl->get_count() > 0);
      scheduler->new_work_agg(node, tpl);
   }
}

void
process::do_work_rules(work& w)
{
   node *node(w.get_node());

	state.run_node(node);
}

void
process::do_work(work& w)
{
   if(w.using_rules()) {
      do_work_rules(w);
      return;
   }

   auto_ptr<const simple_tuple> stuple(w.get_tuple()); // this will delete tuple automatically
   vm::tuple *tuple = stuple->get_tuple();
   ref_count count = stuple->get_count();
   node *node(w.get_node());
   
   if(stuple->must_be_deleted()) {
		delete tuple;
	   return;
	}
	
	assert(count != 0);

#ifdef DEBUG_MODE
   cout << node->get_id() << " " << *tuple << endl;
   
	string name(tuple->get_predicate()->get_name());

   if(name == string("rv"))
      state.print_instrs = true;
#endif
   
   if(w.locally_generated())
      node->pop_auto(stuple.get());

   if(count > 0) {
      if(tuple->is_action())
         do_tuple_action(node, tuple, count);
      else if(tuple->is_aggregate() && !w.force_aggregate())
         do_agg_tuple_add(node, tuple, count);
      else
         do_tuple_add(node, tuple, count);
   } else {
      assert(!tuple->is_action());
      
      count = -count;
      
      if(tuple->is_aggregate() && !w.force_aggregate()) {
         node->remove_agg_tuple(tuple, count);
      } else {
         node::delete_info deleter(node->delete_tuple(tuple, count));
         
         if(deleter.to_delete()) { // to be removed
            state.setup(tuple, node, -count);
            execute_bytecode(state.PROGRAM->get_predicate_bytecode(tuple->get_predicate_id()), state);
            deleter();
         } else
            delete tuple; // as in the positive case, nothing to do
      }
   }
}

void
process::do_loop(void)
{
   work w;
   while(true) {
      while(scheduler->get_work(w)) {
         do_work(w);
         scheduler->finish_work(w);
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
   state(this)
{
}

process::~process(void)
{
   delete thread;
   delete scheduler;
}

}
