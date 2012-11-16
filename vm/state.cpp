
#include "vm/state.hpp"
#include "process/process.hpp"
#include "process/machine.hpp"
#include "vm/exec.hpp"

using namespace vm;
using namespace db;
using namespace process;
using namespace std;
using namespace runtime;

namespace vm
{

state::reg state::consts[MAX_CONSTS];
program *state::PROGRAM = NULL;
database *state::DATABASE = NULL;
machine *state::MACHINE = NULL;
remote *state::REMOTE = NULL;
router *state::ROUTER = NULL;
size_t state::NUM_THREADS = 0;
size_t state::NUM_PREDICATES = 0;
size_t state::NUM_NODES = 0;
size_t state::NUM_NODES_PER_PROCESS = 0;
machine_arguments state::ARGUMENTS;

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
   for(simple_tuple_vector::iterator it(generated_tuples.begin()), end(generated_tuples.end());
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
	consts[cid] = regs[reg_from];
	switch(PROGRAM->get_const_type(cid)) {
		case FIELD_LIST_INT:
			int_list::inc_refs(get_const_int_list(cid)); break;
		case FIELD_LIST_FLOAT:
			float_list::inc_refs(get_const_float_list(cid)); break;
		case FIELD_LIST_NODE:
			node_list::inc_refs(get_const_node_list(cid)); break;
		case FIELD_STRING:
			get_const_string(cid)->inc_refs(); break;
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

void
state::mark_predicate_to_run(const predicate *pred)
{
	if(!predicates[pred->get_id()]) {
		predicates[pred->get_id()] = true;
		predicates_to_check.push_back((predicate*)pred);
	}
}

void
state::mark_rules_using_active_predicates(void)
{
	for(vector<predicate*>::iterator it(predicates_to_check.begin()), end(predicates_to_check.end());
		it != end;
		it++)
	{
		mark_predicate_rules(*it);
	}
}

void
state::mark_rules_using_local_tuples(db::node *node)
{
	predicates_to_check.clear();
	fill_n(predicates, PROGRAM->num_predicates(), false);
	
	for(db::simple_tuple_vector::iterator it(local_tuples.begin());
		it != local_tuples.end(); )
	{
		db::simple_tuple *stpl(*it);
		vm::tuple *tpl(stpl->get_tuple());
		
		if(tpl->is_persistent()) {
			const bool is_new(node->add_tuple(tpl, 1));
			if(is_new)
				mark_predicate_to_run(tpl->get_predicate());
			else
				delete tpl;
			delete stpl;
			it = local_tuples.erase(it);
		} else {
			mark_predicate_to_run(tpl->get_predicate());
			it++;
		}
	}
	
	mark_rules_using_active_predicates();
}

void
state::process_consumed_local_tuples(void)
{
	// process current set of tuples
	for(db::simple_tuple_vector::iterator it(local_tuples.begin());
		it != local_tuples.end();
		)
	{
		simple_tuple *stpl(*it);
		if(!stpl->can_be_consumed()) {
#ifdef DEBUG_RULES
			cout << "Delete " << *stpl << endl;
#endif
			delete stpl->get_tuple();
			delete stpl;
			it = local_tuples.erase(it);
		} else
			it++;
	}
}

void
state::process_generated_tuples(const strat_level current_level, db::node *node)
{
	predicates_to_check.clear();
	fill_n(predicates, PROGRAM->num_predicates(), false);
	
	// get tuples generated with the previous rule
	for(simple_tuple_vector::iterator it(generated_tuples.begin()), end(generated_tuples.end());
		it != end;
		it++)
	{
		simple_tuple *stpl(*it);
		vm::tuple *tpl(stpl->get_tuple());
		const predicate *pred(tpl->get_predicate());
		
		if(pred->get_strat_level() != current_level) {
			assert(stpl->can_be_consumed());
			MACHINE->route_self(proc, node, stpl);
		} else if(pred->is_action_pred()) {
			MACHINE->run_action(proc->get_scheduler(), node, tpl);
			delete tpl;
			delete stpl;
		} else {
			if(tpl->is_persistent()) {
				const bool is_new(node->add_tuple(tpl, 1));
				if(is_new)
					mark_predicate_to_run(pred);
				else
					delete tpl;
				delete stpl;
			} else {
				local_tuples.push_back(stpl);
				mark_predicate_to_run(pred);
			}
		}
	}
	
	mark_rules_using_active_predicates();
}

void
state::run_node(db::node *node)
{
#ifdef DEBUG_RULES
   cout << "Node " << node->get_id() << endl;
#endif

	strat_level level;
	proc->get_scheduler()->gather_next_tuples(node, local_tuples, level);
	mark_rules_using_local_tuples(node);

#ifdef DEBUG_RULES
	cout << "Strat level: " << level << " got " << local_tuples.size() << " tuples " << endl;
#endif

	while(!rule_queue.empty()) {
		rule_id rule(rule_queue.pop());

		setup(NULL, node, 0);
		use_local_tuples = true;
		execute_rule(rule, *this);

		process_consumed_local_tuples();

#ifdef DEBUG_RULES
		cout << "Generated " << generated_tuples.size() << " tuples" << endl;
#endif

		rules[rule] = false;
		process_generated_tuples(level, node);

		generated_tuples.clear();
	}

	for(db::simple_tuple_vector::iterator it(local_tuples.begin()), end(local_tuples.end());
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

	local_tuples.clear();
}

#ifdef CORE_STATISTICS
void
state::init_core_statistics(void)
{
   stat_rules_ok = 0;
   stat_rules_failed = 0;
   stat_db_hits = 0;
   stat_tuples_used = 0;
   stat_if_tests = 0;
	stat_if_failed = 0;
	stat_instructions_executed = 0;
	stat_moves_executed = 0;
	stat_ops_executed = 0;
}
#endif

state::state(process::process *_proc):
   proc(_proc)
#ifdef DEBUG_MODE
   , print_instrs(false)
#endif
{
#ifdef CORE_STATISTICS
   init_core_statistics();
#endif
	rules = new bool[state::PROGRAM->num_rules()];
	fill_n(rules, state::PROGRAM->num_rules(), false);
	predicates = new bool[state::PROGRAM->num_predicates()];
	fill_n(predicates, state::PROGRAM->num_predicates(), false);
	rule_queue.set_type(HEAP_INT_ASC);
}

state::state(void):
   rules(NULL), predicates(NULL), proc(NULL)
#ifdef DEBUG_MODE
   , print_instrs(false)
#endif
{
#ifdef CORE_STATISTICS
   init_core_statistics();
#endif
}

state::~state(void)
{
#ifdef CORE_STATISTICS
	if(proc != NULL) {
      return;
		cout << "Rules:" << endl;
   	cout << "\tapplied: " << stat_rules_ok << endl;
   	cout << "\tfailed: " << stat_rules_failed << endl;
		cout << "DB hits: " << stat_db_hits << endl;
		cout << "Tuples used: " << stat_tuples_used << endl;
		cout << "If tests: " << stat_if_tests << endl;
		cout << "\tfailed: " << stat_if_failed << endl;
		cout << "Instructions executed: " << stat_instructions_executed << endl;
		cout << "\tmoves: " << stat_moves_executed << endl;
		cout << "\tops: " << stat_ops_executed << endl;
	}
#endif
	delete []rules;
	delete []predicates;
	assert(rule_queue.empty());
}

}
