
#ifndef VM_STATE_HPP
#define VM_STATE_HPP

#include <list>

#include "conf.hpp"
#include "vm/tuple.hpp"
#include "db/node.hpp"
#include "vm/program.hpp"
#include "vm/instr.hpp"
#include "db/trie.hpp"
#include "utils/random.hpp"
#include "queue/safe_simple_pqueue.hpp"

// forward declaration
namespace process {
   class remote;
   class machine;
   class router;
}

namespace sched {
	class base;
}

namespace db {
   class database;
}

namespace vm {
	
#define MAX_CONSTS 32

typedef std::vector<std::string> machine_arguments;

class state
{
private:
   
   static const size_t NUM_REGS = 32;
   typedef tuple_field reg;
   reg regs[NUM_REGS];
   db::tuple_trie_leaf *saved_leafs[NUM_REGS];
	db::simple_tuple *saved_stuples[NUM_REGS];
	bool is_leaf[NUM_REGS];
	
	std::list<runtime::float_list*, mem::allocator<runtime::float_list*> > free_float_list;
   std::list<runtime::int_list*, mem::allocator<runtime::int_list*> > free_int_list;
   std::list<runtime::node_list*, mem::allocator<runtime::node_list*> > free_node_list;
	std::list<runtime::rstring::ptr, mem::allocator<runtime::rstring::ptr> > free_rstring;
   
   typedef std::pair<vm::tuple *, vm::ref_count> pair_linear;
   typedef std::list<pair_linear> list_linear;

	static reg consts[MAX_CONSTS];
	
	/* execution data for when using rules */
	bool *rules;
	bool *predicates;
	queue::heap_queue<vm::rule_id> rule_queue;
#ifndef USE_RULE_COUNTING
	std::vector<vm::predicate*> predicates_to_check;
#endif
	
   void purge_runtime_objects(void);
#ifdef CORE_STATISTICS
   void init_core_statistics(void);
#endif

public:
	
#define define_get_const(WHAT, TYPE, CODE) static TYPE get_const_ ## WHAT (const const_id& id) { return CODE; }
	
	define_get_const(int, int_val, *(int_val*)(consts + id))
	define_get_const(float, float_val, *(float_val*)(consts + id))
	define_get_const(ptr, ptr_val, *(ptr_val*)(consts + id));
	define_get_const(int_list, runtime::int_list*, (runtime::int_list*)get_const_ptr(id))
	define_get_const(float_list, runtime::float_list*, (runtime::float_list*)get_const_ptr(id))
	define_get_const(node_list, runtime::node_list*, (runtime::node_list*)get_const_ptr(id))
	define_get_const(string, runtime::rstring::ptr, (runtime::rstring::ptr)get_const_ptr(id))
	define_get_const(node, node_val, *(node_val*)(consts + id))
	
#undef define_get_const
	
#define define_set_const(WHAT, TYPE, CODE) static void set_const_ ## WHAT (const const_id& id, const TYPE val) { CODE;}
	
	define_set_const(int, int_val&, *(int_val*)(consts + id) = val)
	define_set_const(float, float_val&, *(float_val*)(consts + id) = val)
	define_set_const(node, node_val&, *(node_val*)(consts +id) = val)
	
#undef define_set_const
	
	vm::tuple *original_tuple;
	enum {
		ORIGINAL_CAN_BE_USED,
		ORIGINAL_CANNOT_BE_USED,
		ORIGINAL_WAS_CONSUMED
	} original_status;
   vm::tuple *tuple;
   db::tuple_trie_leaf *tuple_leaf;
	db::simple_tuple *tuple_queue;
   db::node *node;
   ref_count count;
   sched::base *sched;
   bool is_linear;
   list_linear used_linear_tuples;
	utils::randgen randgen;
   size_t current_rule;
#ifdef DEBUG_MODE
	bool print_instrs;
#endif
   bool use_local_tuples;
   db::simple_tuple_list local_tuples;
	db::simple_tuple_list generated_tuples;
	db::simple_tuple_vector generated_persistent_tuples;
	db::simple_tuple_vector generated_other_level;
	vm::strat_level current_level;
   
   static program *PROGRAM;
   static db::database *DATABASE;
   static process::machine *MACHINE;
   static process::remote *REMOTE;
   static process::router *ROUTER;
   static size_t NUM_THREADS;
   static size_t NUM_PREDICATES;
   static size_t NUM_NODES;
   static size_t NUM_NODES_PER_PROCESS;
	static machine_arguments ARGUMENTS;
   static double TASK_STEALING_FACTOR;
#ifdef USE_UI
   static bool UI;
#endif

#ifdef CORE_STATISTICS
   size_t stat_rules_ok;
   size_t stat_rules_failed;
   size_t stat_db_hits;
   size_t stat_tuples_used;
   size_t stat_if_tests;
	size_t stat_if_failed;
	size_t stat_instructions_executed;
	size_t stat_moves_executed;
	size_t stat_ops_executed;
   size_t *stat_predicate_proven;
   size_t *stat_predicate_applications;
   size_t *stat_predicate_success;

	bool stat_inside_rule;
	size_t stat_rules_activated;
#endif
   
#define define_get(WHAT, RET, BODY) \
   inline RET get_ ## WHAT (const reg_num& num) const { BODY; }
   
   define_get(reg, reg, return regs[num]);
   define_get(int, int_val, return regs[num].int_field);
   define_get(float, float_val, return regs[num].float_field);
   define_get(ptr, ptr_val, return regs[num].ptr_field);
   define_get(bool, bool_val, return get_int(num) ? true : false);
	define_get(string, runtime::rstring::ptr, return (runtime::rstring::ptr)get_ptr(num););
   define_get(int_list, runtime::int_list*, return (runtime::int_list*)get_ptr(num));
   define_get(float_list, runtime::float_list*, return (runtime::float_list*)get_ptr(num));
   define_get(node_list, runtime::node_list*, return (runtime::node_list*)get_ptr(num));
   define_get(tuple, vm::tuple*, return (vm::tuple*)get_ptr(num));
   define_get(node, vm::node_val, return regs[num].node_field);
   
#undef define_get

#define define_set(WHAT, ARG, BODY) 												\
   inline void set_ ## WHAT (const reg_num& num, ARG val) { BODY; };
   
   define_set(float, const float_val&, regs[num].float_field = val);
   define_set(int, const int_val&, regs[num].int_field = val);
   define_set(ptr, const ptr_val&, regs[num].ptr_field = val);
   define_set(bool, const bool_val&, set_int(num, val ? 1 : 0));
	define_set(string, const runtime::rstring::ptr, set_ptr(num, (ptr_val)val));
   define_set(int_list, runtime::int_list*, set_ptr(num, (ptr_val)val));
   define_set(float_list, runtime::float_list*, set_ptr(num, (ptr_val)val));
   define_set(node_list, runtime::node_list*, set_ptr(num, (ptr_val)val));
   define_set(tuple, vm::tuple*, set_ptr(num, (ptr_val)val));
   define_set(node, const node_val, regs[num].node_field = val);
   
#undef define_set
   
   inline void set_nil(const reg_num& num) { set_ptr(num, null_ptr_val); }
   
   inline void set_leaf(const reg_num& num, db::tuple_trie_leaf* leaf) { is_leaf[num] = true; saved_leafs[num] = leaf; }
   inline db::tuple_trie_leaf* get_leaf(const reg_num& num) const { return saved_leafs[num]; }
	inline void set_tuple_queue(const reg_num& num, db::simple_tuple *stpl) { is_leaf[num] = false; saved_stuples[num] = stpl; }
	inline db::simple_tuple* get_tuple_queue(const reg_num& num) const { return saved_stuples[num]; }
	inline bool is_it_a_leaf(const reg_num& num) const { return is_leaf[num]; }
   
   inline void copy_reg(const reg_num& reg_from, const reg_num& reg_to) {
      regs[reg_to] = regs[reg_from];
   }

	void copy_reg2const(const reg_num&, const const_id&);
   
   inline void add_float_list(runtime::float_list *ls) { free_float_list.push_back(ls); }
   inline void add_int_list(runtime::int_list *ls) { free_int_list.push_back(ls); }
   inline void add_node_list(runtime::node_list *ls) { free_node_list.push_back(ls); }
	inline void add_string(runtime::rstring::ptr str) { free_rstring.push_back(str); }
   inline void add_generated_tuple(db::simple_tuple *tpl) { tpl->set_generated_run(true); generated_tuples.push_back(tpl); }
   
	bool add_fact_to_node(vm::tuple *);
	
#ifndef USE_RULE_COUNTING
	void mark_predicate_rules(const vm::predicate *);
#else
	bool check_if_rule_predicate_activated(vm::rule *);
#endif
	
	void mark_predicate_to_run(const vm::predicate *);
	void mark_active_rules(void);
	void process_consumed_local_tuples(void);
	void process_generated_tuples(void);
	void mark_rules_using_local_tuples(void);
	void run_node(db::node *);
   void setup(vm::tuple*, db::node*, const ref_count);
   void cleanup(void);
   bool linear_tuple_can_be_used(vm::tuple *, const vm::ref_count) const;
   void using_new_linear_tuple(vm::tuple *);
   void no_longer_using_linear_tuple(vm::tuple *);
   void unmark_generated_tuples(void);

	static inline runtime::rstring::ptr get_argument(const argument_id id)
	{
		assert(id <= ARGUMENTS.size());
		return runtime::rstring::make_string(ARGUMENTS[id-1]);
	}
	
   explicit state(sched::base *);
	explicit state(void);
   ~state(void);
};

}

#endif
