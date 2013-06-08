
#include "vm/state.hpp"
#include "process/machine.hpp"
#include "vm/exec.hpp"
#ifdef USE_SIM
#include "sched/nodes/sim.hpp"
#endif

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
#ifdef USE_SIM
bool state::SIM = false;
#endif

bool
state::linear_tuple_can_be_used(db::tuple_trie_leaf *leaf) const
{
   assert(leaf != NULL);

   for(list_linear::const_iterator it(used_linear_tuples.begin()), end(used_linear_tuples.end());
      it != end;
      it++)
   {
      const pair_linear& p(*it);
      
      if(p.first == leaf)
         return p.second > 0;
   }
   
   return true; // not found, first time
}

void
state::using_new_linear_tuple(db::tuple_trie_leaf *leaf)
{
   for(list_linear::iterator it(used_linear_tuples.begin()), end(used_linear_tuples.end());
      it != end;
      it++)
   {
      pair_linear& p(*it);
      
      if(p.first == leaf) {
         assert(p.second > 0);
         p.second--;
         return;
      }
   }
   
   // first time, get the count
   used_linear_tuples.push_front(pair_linear(leaf, leaf->get_count() - 1));
}

void
state::no_longer_using_linear_tuple(db::tuple_trie_leaf *leaf)
{
   for(list_linear::iterator it(used_linear_tuples.begin()), end(used_linear_tuples.end());
      it != end;
      it++)
   {
      pair_linear &p(*it);
      
      if(p.first == leaf) {
         p.second++; // a free count
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
   used_linear_tuples.clear();
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

static inline bool
tuple_for_assertion(db::simple_tuple *stpl)
{
   return ((stpl->get_tuple())->is_aggregate() && stpl->is_aggregate())
      || !stpl->get_tuple()->is_aggregate();
}

db::simple_tuple*
state::search_for_negative_tuple_partial_agg(db::simple_tuple *stpl)
{
   vm::tuple *tpl(stpl->get_tuple());

   assert(!stpl->is_aggregate());

   for(db::simple_tuple_list::iterator it(generated_persistent_tuples.begin());
         it != generated_persistent_tuples.end(); ++it)
   {
      db::simple_tuple *stpl2(*it);
      vm::tuple *tpl2(stpl2->get_tuple());

      if(tpl2->is_aggregate() && !stpl2->is_aggregate() &&
            stpl2->get_count() == -1 && *tpl2 == *tpl)
      {
         generated_persistent_tuples.erase(it);
         return stpl2;
      }
   }

   return NULL;
}

db::simple_tuple*
state::search_for_negative_tuple_normal(db::simple_tuple *stpl)
{
   vm::tuple *tpl(stpl->get_tuple());

   assert(!tpl->is_aggregate());
   assert(!stpl->is_aggregate());

   for(db::simple_tuple_list::iterator it(generated_persistent_tuples.begin());
         it != generated_persistent_tuples.end(); ++it)
   {
      db::simple_tuple *stpl2(*it);
      vm::tuple *tpl2(stpl2->get_tuple());

      if(!tpl2->is_aggregate() && stpl2->get_count() == -1 && *tpl2 == *tpl)
      {
         generated_persistent_tuples.erase(it);
         return stpl2;
      }
   }

   return NULL;
}

db::simple_tuple*
state::search_for_negative_tuple_full_agg(db::simple_tuple *stpl)
{
   vm::tuple *tpl(stpl->get_tuple());

   for(db::simple_tuple_list::iterator it(generated_persistent_tuples.begin());
         it != generated_persistent_tuples.end(); ++it)
   {
      db::simple_tuple *stpl2(*it);
      vm::tuple *tpl2(stpl2->get_tuple());

      if(tpl2->is_aggregate() && stpl2->is_aggregate() && stpl2->get_count() == -1 && *tpl2 == *tpl)
      {
         generated_persistent_tuples.erase(it);
         return stpl2;
      }
   }

   return NULL;
}

#ifdef USE_SIM
bool
state::check_instruction_limit(void) const
{
   return sim_instr_use && sim_instr_counter >= sim_instr_limit;
}
#endif

bool
state::do_persistent_tuples(void)
{
   // we grab the stratification level here
   while(!generated_persistent_tuples.empty()) {
#ifdef USE_SIM
      if(check_instruction_limit()) {
         return false;
      }
#endif
      db::simple_tuple *stpl(generated_persistent_tuples.front());
      vm::tuple *tpl(stpl->get_tuple());

      generated_persistent_tuples.pop_front();

      if(tpl->is_persistent()) {
         // XXX crashes when calling wipeout below
         if(stpl->get_count() == 1 && (tpl->is_aggregate() && !stpl->is_aggregate())) {
            db::simple_tuple *stpl2(search_for_negative_tuple_partial_agg(stpl));
            if(stpl2) {
               assert(stpl != stpl2);
               //assert(stpl2->get_tuple() != stpl->get_tuple());
               //assert(stpl2->get_predicate() == stpl->get_predicate());
               //simple_tuple::wipeout(stpl);
               //simple_tuple::wipeout(stpl2);
               continue;
            }
         }

         if(stpl->get_count() == 1 && (tpl->is_aggregate() && stpl->is_aggregate())) {
            db::simple_tuple *stpl2(search_for_negative_tuple_full_agg(stpl));
            if(stpl2) {
               assert(stpl != stpl2);
               /*if(stpl2->get_tuple() == stpl->get_tuple()) {
                  cout << "fail " << *(stpl2->get_tuple()) << endl;
               }
               assert(stpl2->get_tuple() != stpl->get_tuple());
               assert(stpl2->get_predicate() == stpl->get_predicate());*/
               //simple_tuple::wipeout(stpl);
               //simple_tuple::wipeout(stpl2);
               continue;
            }
         }

         if(stpl->get_count() == 1 && !tpl->is_aggregate()) {
            db::simple_tuple *stpl2(search_for_negative_tuple_normal(stpl));
            if(stpl2) {
               //simple_tuple::wipeout(stpl);
               //simple_tuple::wipeout(stpl2);
               continue;
            }
         }
      }

      if(tuple_for_assertion(stpl)) {
#ifdef DEBUG_RULES
      cout << ">>>>>>>>>>>>> Running process for " << node->get_id() << " " << *stpl << endl;
#endif
         process_persistent_tuple(stpl, tpl);
      } else {
         // aggregate
#ifdef DEBUG_RULES
      cout << ">>>>>>>>>>>>> Adding aggregate " << node->get_id() << " " << *stpl << endl;
#endif
         add_to_aggregate(stpl);
      }
   }

   while(!leaves_for_deletion.empty()) {
      pair<vm::predicate*, db::tuple_trie_leaf*> p(leaves_for_deletion.front());
		vm::predicate* pred(p.first);
		db::tuple_trie_leaf *leaf(p.second);
		
		leaves_for_deletion.pop_front();
		
		node->delete_by_leaf(pred, leaf);
	}
	
	assert(leaves_for_deletion.empty());

   return true;
}

vm::strat_level
state::mark_rules_using_local_tuples(db::simple_tuple_list& ls)
{
   bool has_level(false);
   vm::strat_level level;

	for(db::simple_tuple_list::iterator it(ls.begin());
		it != ls.end(); )
	{
		db::simple_tuple *stpl(*it);
		vm::tuple *tpl(stpl->get_tuple());

      if(!has_level) {
         level = stpl->get_strat_level();
         has_level = true;
      }

      if(!stpl->can_be_consumed()) {
         delete tpl;
         delete stpl;
         it = ls.erase(it);
      } else if(tpl->is_action()) {
         all->MACHINE->run_action(sched, node, tpl);
         delete stpl;
         it = ls.erase(it);
      } else if(tpl->is_persistent() || tpl->is_reused()) {
         generated_persistent_tuples.push_back(stpl);
         it = ls.erase(it);
		} else {
#ifdef USE_RULE_COUNTING
			node->matcher.register_tuple(tpl, stpl->get_count());
#endif
			mark_predicate_to_run(tpl->get_predicate());
			it++;
		}
	}

   return level;
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
      stpl->set_as_aggregate();
      generated_persistent_tuples.push_back(stpl);
   }
}

void
state::process_persistent_tuple(db::simple_tuple *stpl, vm::tuple *tpl)
{
   if(stpl->get_count() > 0) {
		const bool is_new(add_fact_to_node(tpl));

      if(is_new) {
         setup(tpl, node, stpl->get_count());
         use_local_tuples = false;
         persistent_only = true;
         execute_bytecode(all->PROGRAM->get_predicate_bytecode(tpl->get_predicate_id()), *this);
      }

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
		if(tpl->is_reused()) {
			setup(tpl, node, stpl->get_count());
			persistent_only = true;
			use_local_tuples = false;
			execute_bytecode(all->PROGRAM->get_predicate_bytecode(tuple->get_predicate_id()), *this);
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
         	delete tpl;
		}
   }
}

void
state::process_others(void)
{
	for(simple_tuple_vector::iterator it(generated_other_level.begin()), end(generated_other_level.end());
		it != end;
		it++)
	{
		/* no need to mark tuples */
		all->MACHINE->route_self(sched, node, *it);
	}

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
   bool aborted(false);

	this->node = no;
	
#ifdef DEBUG_RULES
   cout << "Node " << node->get_id() << endl;
#endif

   assert(local_tuples.empty());
	sched->gather_next_tuples(node, local_tuples);
   start_matching();
	current_level = mark_rules_using_local_tuples(local_tuples);
#ifdef DEBUG_RULES
	cout << "Strat level: " << current_level << " got " << local_tuples.size() << " tuples " << endl;
#endif

   if(do_persistent_tuples()) {
      mark_active_rules();
   } else
      // if using the simulator, we check if we exhausted the available time to run
      aborted = true;

	while(!rule_queue.empty() && !aborted) {
#ifdef USE_SIM
      if(check_instruction_limit()) {
         break;
      }
#endif

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
#ifdef USE_SIM
      if(sim_instr_use && !check_instruction_limit()) {
         // gather new tuples
         db::simple_tuple_list new_tuples;
         sched::sim_node *snode(dynamic_cast<sched::sim_node*>(node));
         snode->get_tuples_until_timestamp(new_tuples, sim_instr_limit);
         local_tuples.splice(local_tuples.end(), new_tuples);
      }
#endif
      /* move from generated tuples to local_tuples */
      local_tuples.splice(local_tuples.end(), generated_tuples);
      if(!do_persistent_tuples()) {
         aborted = true;
         break;
      }
		mark_active_rules();
	}

   // push remaining tuples into node
	for(db::simple_tuple_list::iterator it(local_tuples.begin()), end(local_tuples.end());
		it != end;
		++it)
	{
		simple_tuple *stpl(*it);
		vm::tuple *tpl(stpl->get_tuple());
		
		if(stpl->can_be_consumed()) {
         if(aborted) {
#ifdef USE_SIM
            sched::sim_node *snode(dynamic_cast<sched::sim_node*>(node));
            snode->pending.push(stpl);
#else
            assert(false);
#endif
         } else {
            add_fact_to_node(tpl);
            delete stpl;
         }
		} else {
         db::simple_tuple::wipeout(stpl);
      }
	}

	local_tuples.clear();
   rule_queue.clear();

   // push other level tuples
   process_others();

#ifdef USE_SIM
   // store any remaining persistent tuples
   sched::sim_node *snode(dynamic_cast<sched::sim_node*>(node));
   for(simple_tuple_list::iterator it(generated_persistent_tuples.begin()), end(generated_persistent_tuples.end());
         it != end;
         ++it)
   {
      assert(aborted);
      db::simple_tuple *stpl(*it);
      snode->pending.push(stpl);
   }
#endif
	
   generated_persistent_tuples.clear();
#ifdef USE_SIM
   if(sim_instr_use && sim_instr_counter < sim_instr_limit)
      ++sim_instr_counter;
#endif
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
#ifdef USE_SIM
   sim_instr_use = false;
#endif
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
#ifdef USE_SIM
   sim_instr_use = false;
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
