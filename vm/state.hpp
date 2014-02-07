
#ifndef VM_STATE_HPP
#define VM_STATE_HPP

#include <list>
#include <unordered_set>

#include "conf.hpp"
#include "vm/tuple.hpp"
#include "db/node.hpp"
#include "vm/program.hpp"
#include "vm/instr.hpp"
#include "db/trie.hpp"
#include "vm/all.hpp"
#include "utils/random.hpp"
#include "utils/time.hpp"
#include "runtime/objs.hpp"
#include "queue/safe_simple_pqueue.hpp"
#include "vm/stat.hpp"
#include "vm/call_stack.hpp"
#include "vm/temporary.hpp"
#include "db/linear_store.hpp"

#define USE_TEMPORARY_STORE

// forward declaration
namespace sched {
	class base;
}

namespace vm {
	
typedef size_t deterministic_timestamp;

struct state
{
private:
   
   db::tuple_trie_leaf *saved_leaves[NUM_REGS];
	bool is_leaf[NUM_REGS];
	
	std::list<runtime::cons*, mem::allocator<runtime::cons*> > free_cons;
	std::list<runtime::rstring::ptr, mem::allocator<runtime::rstring::ptr> > free_rstring;
   std::list<runtime::struct1*, mem::allocator<runtime::struct1*> > free_struct1;
   
   typedef std::pair<db::tuple_trie_leaf *, vm::ref_count> pair_linear;
   typedef std::list<pair_linear> list_linear;

	/* execution data for when using rules */
	queue::heap_queue<vm::rule_id> rule_queue;
	
   void purge_runtime_objects(void);
   db::simple_tuple* search_for_negative_tuple_partial_agg(db::simple_tuple *);
   db::simple_tuple* search_for_negative_tuple_full_agg(db::simple_tuple *);
   db::simple_tuple* search_for_negative_tuple_normal(db::simple_tuple *);

public:

   typedef tuple_field reg;
   reg regs[NUM_REGS];
   vm::predicate *preds[NUM_REGS];
   call_stack stack;
   db::node *node;
   derivation_count count;
   vm::depth_t depth;
   sched::base *sched;
   bool is_linear;
	utils::randgen randgen;
   size_t current_rule;
#ifdef DEBUG_MODE
	bool print_instrs;
#endif

   size_t linear_facts_generated;
   size_t persistent_facts_generated;
   size_t linear_facts_consumed;
   bool generated_facts;
   bool running_rule;
   bool hash_removes;
   typedef std::unordered_set<vm::tuple*, std::hash<vm::tuple*>, std::equal_to<vm::tuple*>, mem::allocator<vm::tuple*> > removed_hash;
   removed_hash removed;
   typedef std::list<match*> match_list;
   match_list matches_created;
   temporary_store *store;
   db::linear_store *lstore;
#ifdef CORE_STATISTICS
   core_statistics stat;
#endif
#ifdef USE_UI
   static bool UI;
#endif
#ifdef USE_SIM
   static bool SIM;
   deterministic_timestamp sim_instr_counter;
   deterministic_timestamp sim_instr_limit;
   bool sim_instr_use;
#endif

#define define_get(WHAT, RET, BODY) \
   inline RET get_ ## WHAT (const reg_num& num) const { BODY; }
   
   define_get(reg, reg, return regs[num]);
   define_get(int, int_val, return FIELD_INT(regs[num]));
   define_get(float, float_val, return FIELD_FLOAT(regs[num]));
   define_get(ptr, ptr_val, return FIELD_PTR(regs[num]));
   define_get(bool, bool_val, return FIELD_BOOL(regs[num]));
	define_get(string, runtime::rstring::ptr, return FIELD_STRING(regs[num]););
   define_get(cons, runtime::cons*, return FIELD_CONS(regs[num]));
   define_get(tuple, vm::tuple*, return (vm::tuple*)get_ptr(num));
   define_get(node, vm::node_val, return FIELD_NODE(regs[num]));
   define_get(struct, runtime::struct1*, return FIELD_STRUCT(regs[num]));
   
#undef define_get

#define define_set(WHAT, ARG, BODY) 												\
   inline void set_ ## WHAT (const reg_num& num, ARG val) { BODY; };
   
   define_set(float, const float_val&, SET_FIELD_FLOAT(regs[num], val));
   define_set(int, const int_val&, SET_FIELD_INT(regs[num], val));
   define_set(ptr, const ptr_val&, SET_FIELD_PTR(regs[num], val));
   define_set(bool, const bool_val&, SET_FIELD_BOOL(regs[num], val));
	define_set(string, const runtime::rstring::ptr, SET_FIELD_STRING(regs[num], val));
   define_set(cons, runtime::cons*, SET_FIELD_CONS(regs[num], val));
   define_set(tuple, vm::tuple*, set_ptr(num, (ptr_val)val));
   define_set(node, const node_val, SET_FIELD_NODE(regs[num], val));
   define_set(struct, runtime::struct1*, SET_FIELD_STRUCT(regs[num], val));
   
#undef define_set
   
   inline void set_reg(const reg_num& num, const reg val) { regs[num] = val; }
   inline void set_nil(const reg_num& num) { set_ptr(num, null_ptr_val); }
   inline reg get_reg(const reg_num& num) { return regs[num]; }
   
   inline void copy_reg(const reg_num& reg_from, const reg_num& reg_to) {
      regs[reg_to] = regs[reg_from];
   }

	void copy_reg2const(const reg_num&, const const_id&);
   
   inline void add_cons(runtime::cons *ls) { ls->inc_refs();
                                             free_cons.push_back(ls); }
	inline void add_string(runtime::rstring::ptr str) { str->inc_refs();
                                                       free_rstring.push_back(str); }
   inline void add_struct(runtime::struct1 *s) { s->inc_refs();
                                                 free_struct1.push_back(s); }
   
	bool add_fact_to_node(vm::tuple *, vm::predicate *, const vm::derivation_count count = 1, const vm::depth_t depth = 0);
	
	void mark_active_rules(void);
   void add_to_aggregate(db::simple_tuple *);
   bool do_persistent_tuples(void);
   void process_persistent_tuple(db::simple_tuple *, vm::tuple *);
	void process_consumed_local_tuples(void);
#ifdef USE_SIM
   bool check_instruction_limit(void) const;
#endif
   void process_action_tuples(void);
   void process_incoming_tuples(void);
	void run_node(db::node *);
   void setup(vm::predicate *, db::node*, const vm::derivation_count, const vm::depth_t);
   void cleanup(void);

   explicit state(sched::base *);
	explicit state(void);
   ~state(void);
};

}

#endif
