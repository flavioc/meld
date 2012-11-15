
#include <iostream>
#include <boost/function.hpp>
#include <boost/thread/tss.hpp>

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

//#define DEBUG_RULES

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
process::mark_predicate_rules(const predicate *pred)
{
	for(predicate::rule_iterator it(pred->begin_rules()), end(pred->end_rules()); it != end; it++)
      rules[*it] = true;
}

void
process::mark_rules_using_local_tuples(db::node *node)
{
	for(db::simple_tuple_vector::iterator it(state.local_tuples.begin());
		it != state.local_tuples.end(); )
	{
		db::simple_tuple *stpl(*it);
		vm::tuple *tpl(stpl->get_tuple());
		
		if(tpl->is_persistent()) {
			const bool is_new(node->add_tuple(tpl, 1));
			if(is_new)
				mark_predicate_rules(tpl->get_predicate());
			else
				delete tpl;
			delete stpl;
			it = state.local_tuples.erase(it);
		} else {
			mark_predicate_rules(tpl->get_predicate());
			it++;
		}
	}
}

void
process::process_consumed_local_tuples(void)
{
	
	// process current set of tuples
	for(db::simple_tuple_vector::iterator it(state.local_tuples.begin());
		it != state.local_tuples.end();
		)
	{
		simple_tuple *stpl(*it);
		if(!stpl->can_be_consumed()) {
#ifdef DEBUG_RULES
			cout << "Delete " << *stpl << endl;
#endif
			delete stpl->get_tuple();
			delete stpl;
			it = state.local_tuples.erase(it);
		} else
			it++;
	}
}

void
process::process_generated_tuples(rule_id &r, const strat_level current_level, db::node *node)
{
	// get tuples generated with the previous rule
	for(simple_tuple_vector::iterator it(state.generated_tuples.begin()), end(state.generated_tuples.end());
		it != end;
		it++)
	{
		simple_tuple *stpl(*it);
		vm::tuple *tpl(stpl->get_tuple());
		const predicate *pred(tpl->get_predicate());
		
		if(pred->get_strat_level() != current_level) {
			assert(stpl->can_be_consumed());
			state::MACHINE->route_self(this, node, stpl);
		} else if(pred->is_action_pred()) {
			state::MACHINE->run_action(scheduler, node, tpl);
			delete tpl;
			delete stpl;
		} else {
			if(tpl->is_persistent()) {
				const bool is_new(node->add_tuple(tpl, 1));
				if(is_new) {
					const predicate *pred(tpl->get_predicate());

		         for(predicate::rule_iterator it(pred->begin_rules()), end(pred->end_rules()); it != end; it++) {
						rule_id rule(*it);
						if(!rules[rule]) {
							rules[rule] = true;
							if(rule < r)
								r = rule;
						}
					}
				} else
					delete tpl;
				delete stpl;
			} else {
				state.local_tuples.push_back(stpl);
      		for(predicate::rule_iterator it(pred->begin_rules()), end(pred->end_rules()); it != end; it++) {
					rule_id rule(*it);
					if(!rules[rule]) {
						rules[rule] = true;
						if(rule < r)
							r = rule;
					}
				}
			}
		}
	}
}

void
process::do_work_rules(work& w)
{
   node *node(w.get_node());

#ifdef DEBUG_RULES
   cout << "Node " << node->get_id() << endl;
#endif
	
	strat_level level;
	scheduler->gather_next_tuples(node, state.local_tuples, level);
	mark_rules_using_local_tuples(node);

#ifdef DEBUG_RULES
	cout << "Strat level: " << level << " got " << state.local_tuples.size() << " tuples " << endl;
#endif

	for(rule_id i(0), end_rule((rule_id)rules.size()); i < end_rule; ) {
		if(!rules[i]) {
			i++;
			continue;
		}
		
		state.setup(NULL, node, 0);
		state.use_local_tuples = true;
		execute_rule(i, state);
	
		process_consumed_local_tuples();
		
		// deactivate rule
		rules[i] = false;
		
#ifdef DEBUG_RULES
		cout << "Generated " << state.generated_tuples.size() << " tuples" << endl;
#endif
		i++;
		process_generated_tuples(i, level, node);
		
		state.generated_tuples.clear();
	}
	
	for(db::simple_tuple_vector::iterator it(state.local_tuples.begin()), end(state.local_tuples.end());
		it != end;
		++it)
	{
		simple_tuple *stpl(*it);
		if(stpl->can_be_consumed()) {
			node->add_tuple(stpl->get_tuple(), 1);
		} else
			delete stpl->get_tuple();
		delete stpl;
	}
	
	state.local_tuples.clear();
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
	rules.resize(state::PROGRAM->num_rules());
	for(size_t i(0); i < rules.size(); i++)
		rules[i] = false;
}

process::~process(void)
{
   delete thread;
   delete scheduler;
}

}
