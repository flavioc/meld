
#include "vm/state.hpp"
#include "process/machine.hpp"
#include "vm/exec.hpp"

using namespace vm;
using namespace db;
using namespace process;
using namespace std;
using namespace runtime;

//#define DEBUG_RULES

namespace vm
{

#ifdef USE_UI
bool state::UI = false;
#endif

bool
state::linear_tuple_can_be_used(vm::tuple *tpl, const vm::ref_count max) const
{
   for(list_linear::const_iterator it(used_linear_tuples.begin()), end(used_linear_tuples.end());
      it != end;
      it++)
   {
      const pair_linear& p(*it);
      
      if(p.first == tpl)
         return p.second < max;
   }
   
   return true; // not found, first time
}

void
state::using_new_linear_tuple(vm::tuple *tpl)
{
   for(list_linear::iterator it(used_linear_tuples.begin()), end(used_linear_tuples.end());
      it != end;
      it++)
   {
      pair_linear& p(*it);
      
      if(p.first == tpl) {
         p.second++;
         return;
      }
   }
   
   // new
   used_linear_tuples.push_front(pair_linear(tpl, 1));
}

void
state::no_longer_using_linear_tuple(vm::tuple *tpl)
{
   for(list_linear::iterator it(used_linear_tuples.begin()), end(used_linear_tuples.end());
      it != end;
      it++)
   {
      pair_linear &p(*it);
      
      if(p.first == tpl) {
         p.second--;
         if(p.second == 0)
            used_linear_tuples.erase(it);
         return;
      }
   }
   
   assert(false);
}

void
state::purge_runtime_objects(void)
{
#define PURGE_OBJ(TYPE) \
   for(list<TYPE*>::iterator it(free_ ## TYPE.begin()); it != free_ ## TYPE .end(); ++it) { \
      TYPE *x(*it); \
      assert(x != NULL); \
		if(x->zero_refs()) { x->destroy(); } \
   } \
   free_ ## TYPE .clear()

#define PURGE_LIST(TYPE) \
	PURGE_OBJ(TYPE ## _list)

   PURGE_LIST(float);
   PURGE_LIST(int);
   PURGE_LIST(node);
#undef PURGE_LIST
	
	PURGE_OBJ(rstring);
	
#undef PURGE_OBJ
}

void
state::unmark_generated_tuples(void)
{
   for(simple_tuple_list::iterator it(generated_tuples.begin()), end(generated_tuples.end());
         it != end;
         ++it)
   {
      simple_tuple *stpl(*it);
      stpl->set_generated_run(false);
   }

   generated_tuples.clear();
}

void
state::cleanup(void)
{
   purge_runtime_objects();
   assert(used_linear_tuples.empty());
}

void
state::copy_reg2const(const reg_num& reg_from, const const_id& cid)
{
   all->set_const(cid, regs[reg_from]);
	switch(all->PROGRAM->get_const_type(cid)) {
		case FIELD_LIST_INT:
			int_list::inc_refs(all->get_const_int_list(cid)); break;
		case FIELD_LIST_FLOAT:
			float_list::inc_refs(all->get_const_float_list(cid)); break;
		case FIELD_LIST_NODE:
			node_list::inc_refs(all->get_const_node_list(cid)); break;
		case FIELD_STRING:
			all->get_const_string(cid)->inc_refs(); break;
		default: break;
	}
}

void
state::setup(vm::tuple *tpl, db::node *n, const ref_count count)
{
   this->use_local_tuples = false;
	this->original_tuple = tpl;
   this->original_status = ORIGINAL_CANNOT_BE_USED;
   this->tuple = tpl;
   this->tuple_leaf = NULL;
   this->node = n;
   this->count = count;
	if(tpl != NULL)
   	this->is_linear = tpl->is_linear();
	else
		this->is_linear = false;
   assert(used_linear_tuples.empty());
   this->used_linear_tuples.clear();
	for(size_t i(0); i < NUM_REGS; ++i) {
		this->is_leaf[i] = false;
		this->saved_leafs[i] = NULL;
		this->saved_stuples[i] = NULL;
	}
#ifdef CORE_STATISTICS
   this->stat_rules_activated = 0;
	this->stat_inside_rule = false;
#endif
}

#ifndef USE_RULE_COUNTING
void
state::mark_predicate_to_run(const predicate *pred)
{
	if(!predicates[pred->get_id()]) {
		predicates[pred->get_id()] = true;
		predicates_to_check.push_back((predicate*)pred);
	}
}

void
state::mark_predicate_rules(const predicate *pred)
{
	for(predicate::rule_iterator it(pred->begin_rules()), end(pred->end_rules()); it != end; it++) {
		rule_id rule(*it);
		if(!rules[rule]) {
			rules[rule] = true;
			heap_priority pr;
			pr.int_priority = (int)rule;
			rule_queue.insert(rule, pr);
		}
	}
}
#else
void
state::mark_predicate_to_run(const predicate *pred)
{
	if(!predicates[pred->get_id()])
		predicates[pred->get_id()] = true;
}

bool
state::check_if_rule_predicate_activated(vm::rule *rule)
{
	for(rule::predicate_iterator jt(rule->begin_predicates()), endj(rule->end_predicates());
		jt != endj;
		jt++)
	{
		const predicate *pred(*jt);
		if(predicates[pred->get_id()])
			return true;
	}
	
	return false;
}
#endif

void
state::mark_active_rules(void)
{
#ifdef USE_RULE_COUNTING
	for(rule_matcher::rule_iterator it(node->matcher.begin_active_rules()),
		end(node->matcher.end_active_rules());
		it != end;
		it++)
	{
		rule_id rid(*it);
		if(!rules[rid]) {
			// we need check if at least one predicate was activated in this loop
			vm::rule *rule(all->PROGRAM->get_rule(rid));
			if(check_if_rule_predicate_activated(rule)) {
				rules[rid] = true;
				heap_priority pr;
				pr.int_priority = (int)rid;
				rule_queue.insert(rid, pr);
			}
		}
	}
	
	for(rule_matcher::rule_iterator it(node->matcher.begin_dropped_rules()),
		end(node->matcher.end_dropped_rules());
		it != end;
		it++)
	{
		rule_id rid(*it);
		if(rules[rid]) {
			rules[rid] = false;
         heap_priority pr;
         pr.int_priority = (int)rid;
			rule_queue.remove(rid, pr);
		}
	}
#else
	for(vector<predicate*>::iterator it(predicates_to_check.begin()), end(predicates_to_check.end());
		it != end;
		it++)
	{
		mark_predicate_rules(*it);
	}
#endif
}

bool
state::add_fact_to_node(vm::tuple *tpl, vm::ref_count count)
{
	return node->add_tuple(tpl, count);
}

void
state::do_persistent_tuples(void)
{
   // we grab the stratification level here
   while(!generated_persistent_tuples.empty()) {
      db::simple_tuple *stpl(generated_persistent_tuples.front());
      vm::tuple *tpl(stpl->get_tuple());

      generated_persistent_tuples.pop_front();

      if(!tpl->is_aggregate() || (tpl->is_aggregate() && stpl->get_generated_run())) {
         stpl->set_generated_run(false);
         process_persistent_tuple(stpl, tpl);
      } else {
         // aggregate
         add_to_aggregate(stpl);
      }
   }
}

void
state::mark_rules_using_local_tuples(void)
{
   bool has_level(false);
	
	for(db::simple_tuple_list::iterator it(local_tuples.begin());
		it != local_tuples.end(); )
	{
		db::simple_tuple *stpl(*it);
		vm::tuple *tpl(stpl->get_tuple());

      if(!has_level) {
         current_level = stpl->get_strat_level();
         has_level = true;
      }
		
      if(!stpl->can_be_consumed()) {
         delete tpl;
         delete stpl;
         it = local_tuples.erase(it);
      } else if(tpl->is_persistent()) {
         generated_persistent_tuples.push_back(stpl);
			it = local_tuples.erase(it);
		} else if(tpl->is_action()) {
			assert(false);
		} else {
#ifdef USE_RULE_COUNTING
			node->matcher.register_tuple(tpl, stpl->get_count());
#endif
			mark_predicate_to_run(tpl->get_predicate());
			it++;
		}
	}
}

void
state::process_consumed_local_tuples(void)
{
	// process current set of tuples
	for(db::simple_tuple_list::iterator it(local_tuples.begin());
		it != local_tuples.end();
		)
	{
		simple_tuple *stpl(*it);
		if(!stpl->can_be_consumed()) {
			vm::tuple *tpl(stpl->get_tuple());
#ifdef USE_RULE_COUNTING
			node->matcher.deregister_tuple(tpl, stpl->get_count());
#endif
			delete tpl;
			delete stpl;
			it = local_tuples.erase(it);
		} else
			it++;
	}
}

void
state::add_to_aggregate(db::simple_tuple *stpl)
{
   vm::tuple *tpl(stpl->get_tuple());
   const predicate *pred(tpl->get_predicate());
   vm::ref_count count(stpl->get_count());
   agg_configuration *agg(NULL);

   if(count < 0) {
      agg = node->remove_agg_tuple(tpl, -count);
   } else {
      agg = node->add_agg_tuple(tpl, count);
   }

   simple_tuple_list list;

   agg->generate(pred->get_aggregate_type(), pred->get_aggregate_field(), list);

   for(simple_tuple_list::iterator it(list.begin()); it != list.end(); ++it) {
      simple_tuple *stpl(*it);
      stpl->set_generated_run(true); // mark as true aggregate
      generated_persistent_tuples.push_back(stpl);
   }
}

void
state::process_persistent_tuple(db::simple_tuple *stpl, vm::tuple *tpl)
{
   if(stpl->get_count() > 0) {
      const bool is_new(add_fact_to_node(tpl));

      setup(tpl, node, stpl->get_count());
      use_local_tuples = false;
      persistent_only = true;
      execute_bytecode(all->PROGRAM->get_predicate_bytecode(tpl->get_predicate_id()), *this);

#ifdef USE_RULE_COUNTING
      node->matcher.register_tuple(tpl, stpl->get_count(), is_new);
#endif

      if(is_new) {
         mark_predicate_to_run(tpl->get_predicate());
      } else {
         delete tpl;
      }
      delete stpl;
   } else {
      node::delete_info deleter(node->delete_tuple(tpl, -stpl->get_count()));

#ifdef USE_RULE_COUNTING
      node->matcher.deregister_tuple(tpl, stpl->get_count());
#endif

      if(deleter.to_delete()) { // to be removed
         setup(tpl, node, stpl->get_count());
         persistent_only = true;
         use_local_tuples = false;
         execute_bytecode(all->PROGRAM->get_predicate_bytecode(tuple->get_predicate_id()), *this);
         deleter();
      } else
         delete tuple;
   }
}

void
state::process_generated_tuples(void)
{
	/* move from generated tuples to local_tuples */
	local_tuples.splice(local_tuples.end(), generated_tuples);
	/* tuples were already marked in vm/exec.cpp (execute_send) */
	
	for(simple_tuple_vector::iterator it(generated_other_level.begin()), end(generated_other_level.end());
		it != end;
		it++)
	{
		/* no need to mark tuples */
		all->MACHINE->route_self(sched, node, *it);
	}
	
   do_persistent_tuples();
	
	assert(generated_tuples.empty());
	generated_persistent_tuples.clear();
	generated_other_level.clear();
}

void
state::start_matching(void)
{
#ifndef USE_RULE_COUNTING
	predicates_to_check.clear();
#endif
	fill_n(predicates, all->PROGRAM->num_predicates(), false);
}

void
state::run_node(db::node *no)
{
	this->node = no;
	
#ifdef DEBUG_RULES
   cout << "Node " << node->get_id() << endl;
#endif

	sched->gather_next_tuples(node, local_tuples);
   start_matching();
	mark_rules_using_local_tuples();
   do_persistent_tuples();
	mark_active_rules();

#ifdef DEBUG_RULES
	cout << "Strat level: " << current_level << " got " << local_tuples.size() << " tuples " << endl;
#endif

	while(!rule_queue.empty()) {
		rule_id rule(rule_queue.pop());
		
#ifdef DEBUG_RULES
		cout << "Run rule " << all->PROGRAM->get_rule(rule)->get_string() << endl;
#endif

		/* delete rule and every check */
		rules[rule] = false;
#ifdef USE_RULE_COUNTING
		node->matcher.clear_dropped_rules();
#endif
      start_matching();
		
		setup(NULL, node, 1);
		use_local_tuples = true;
      persistent_only = false;
		execute_rule(rule, *this);

		process_consumed_local_tuples();
		process_generated_tuples();
		mark_active_rules();
	}

	for(db::simple_tuple_list::iterator it(local_tuples.begin()), end(local_tuples.end());
		it != end;
		++it)
	{
		simple_tuple *stpl(*it);
		vm::tuple *tpl(stpl->get_tuple());
		
		if(stpl->can_be_consumed()) {
			add_fact_to_node(tpl);
		} else
			delete tpl;
		delete stpl;
	}

	local_tuples.clear();
}

#ifdef CORE_STATISTICS
void
state::init_core_statistics(void)
{
   if(sched) {
      stat_rules_ok = 0;
      stat_rules_failed = 0;
      stat_db_hits = 0;
      stat_tuples_used = 0;
      stat_if_tests = 0;
      stat_if_failed = 0;
      stat_instructions_executed = 0;
      stat_moves_executed = 0;
      stat_ops_executed = 0;
      stat_predicate_proven = new size_t[PROGRAM->num_predicates()];
      stat_predicate_applications = new size_t[PROGRAM->num_predicates()];
      stat_predicate_success = new size_t[PROGRAM->num_predicates()];
      for(size_t i(0); i < PROGRAM->num_predicates(); ++i) {
         stat_predicate_proven[i] = 0;
         stat_predicate_applications[i] = 0;
         stat_predicate_success[i] = 0;
      }
   }
}
#endif

state::state(sched::base *_sched, vm::all *_all):
   sched(_sched)
#ifdef DEBUG_MODE
   , print_instrs(false)
#endif
   , all(_all)
{
#ifdef CORE_STATISTICS
   init_core_statistics();
#endif
	rules = new bool[all->PROGRAM->num_rules()];
	fill_n(rules, all->PROGRAM->num_rules(), false);
	predicates = new bool[all->PROGRAM->num_predicates()];
	fill_n(predicates, all->PROGRAM->num_predicates(), false);
	rule_queue.set_type(HEAP_INT_ASC);
}

state::state(vm::all *_all):
   rules(NULL), predicates(NULL), sched(NULL)
#ifdef DEBUG_MODE
   , print_instrs(false)
#endif
   , persistent_only(false)
   , all(_all)
{
#ifdef CORE_STATISTICS
   init_core_statistics();
#endif
}

state::~state(void)
{
#ifdef CORE_STATISTICS
	if(sched != NULL) {
		cout << "Rules:" << endl;
   	cout << "\tapplied: " << stat_rules_ok << endl;
   	cout << "\tfailed: " << stat_rules_failed << endl;
      cout << "\tfailure rate: " << setprecision (2) << 100 * (float)stat_rules_failed / ((float)(stat_rules_ok + stat_rules_failed)) << "%" << endl;
		cout << "DB hits: " << stat_db_hits << endl;
		cout << "Tuples used: " << stat_tuples_used << endl;
		cout << "If tests: " << stat_if_tests << endl;
		cout << "\tfailed: " << stat_if_failed << endl;
		cout << "Instructions executed: " << stat_instructions_executed << endl;
		cout << "\tmoves: " << stat_moves_executed << endl;
		cout << "\tops: " << stat_ops_executed << endl;
      cout << "Proven predicates:" << endl;
      for(size_t i(0); i < PROGRAM->num_predicates(); ++i)
         cout << "\t" << PROGRAM->get_predicate(i)->get_name() << " " << stat_predicate_proven[i] << endl;
      cout << "Applications predicate:" << endl;
      for(size_t i(0); i < PROGRAM->num_predicates(); ++i)
         cout << "\t" << PROGRAM->get_predicate(i)->get_name() << " " << stat_predicate_applications[i] << endl;
      cout << "Successes predicate:" << endl;
      for(size_t i(0); i < PROGRAM->num_predicates(); ++i)
         cout << "\t" << PROGRAM->get_predicate(i)->get_name() << " " << stat_predicate_success[i] << endl;
      delete []stat_predicate_proven;
      delete []stat_predicate_applications;
      delete []stat_predicate_success;
	}
#endif
	delete []rules;
	delete []predicates;
	assert(rule_queue.empty());
}

}
