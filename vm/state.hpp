
#ifndef VM_STATE_HPP
#define VM_STATE_HPP

#include <list>
#include <unordered_set>

#include "vm/tuple.hpp"
#include "db/node.hpp"
#include "vm/program.hpp"
#include "vm/instr.hpp"
#include "db/trie.hpp"
#include "vm/all.hpp"
#include "utils/random.hpp"
#include "utils/time.hpp"
#include "utils/hash.hpp"
#include "runtime/objs.hpp"
#include "vm/call_stack.hpp"
#include "db/temporary_store.hpp"
#include "db/linear_store.hpp"
#include "vm/counter.hpp"
#ifdef CORE_STATISTICS
#include "vm/stat.hpp"
#endif

// forward declaration
namespace sched { class threads_sched; }

namespace vm {
	
struct state
{
private:
   
   db::tuple_trie_leaf *saved_leaves[NUM_REGS];
	bool is_leaf[NUM_REGS];
	
	std::list<runtime::cons*, mem::allocator<runtime::cons*> > free_cons;
	std::list<runtime::rstring::ptr, mem::allocator<runtime::rstring::ptr> > free_rstring;
   std::list<runtime::struct1*, mem::allocator<runtime::struct1*> > free_struct1;
   
   void purge_runtime_objects(void);
   full_tuple* search_for_negative_tuple_partial_agg(full_tuple *);
   full_tuple* search_for_negative_tuple_full_agg(full_tuple *);
   full_tuple* search_for_negative_tuple_normal(full_tuple *);

   void indexing_state_machine(db::node *);

public:

   using reg = tuple_field;

   reg regs[NUM_REGS];
   vm::predicate *preds[NUM_REGS];
   call_stack stack;
   db::node *node;
   derivation_count count;
   vm::depth_t depth;
   sched::threads_sched *sched;
   bool is_linear;
	utils::randgen randgen;
   size_t current_rule;
#ifdef FACT_BUFFERING
   using map_sends = std::unordered_map<const db::node*, tuple_array,
         db::node_hash,
         std::equal_to<const db::node*>,
         mem::allocator<std::pair<const db::node* const, tuple_array>>>;

   map_sends facts_to_send;
#endif
#ifdef COORDINATION_BUFFERING
   using map_set_priority = std::unordered_map<const db::node*, vm::priority_t,
         db::node_hash,
         std::equal_to<const db::node*>,
         mem::allocator<std::pair<const db::node* const, vm::priority_t>>>;
   map_set_priority set_priorities;
#endif
#ifdef DEBUG_MODE
	bool print_instrs;
#endif

   size_t linear_facts_generated;
   size_t persistent_facts_generated;
   size_t linear_facts_consumed;
#ifdef INSTRUMENTATION
   size_t instr_facts_consumed;
   size_t instr_facts_derived;
   size_t instr_rules_run;
#endif
   bool generated_facts;
   bool running_rule;
   bool hash_removes;
   using removed_hash =
      std::unordered_set<vm::tuple*, utils::pointer_hash<vm::tuple>,
         std::equal_to<vm::tuple*>, mem::allocator<vm::tuple*>>;
   removed_hash removed;
   db::temporary_store *store;
   db::linear_store *lstore;
   vm::counter *match_counter;
   std::unordered_set<utils::byte*, utils::pointer_hash<utils::byte>,
      std::equal_to<utils::byte*>, mem::allocator<utils::byte*>> allocated_match_objects;
#ifdef GC_NODES
   candidate_gc_nodes gc_nodes;
#endif
#ifdef CORE_STATISTICS
   core_statistics stat;
#endif
#ifdef FACT_STATISTICS
   uint64_t facts_derived;
   uint64_t facts_consumed;
   uint64_t facts_sent;
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
	
   void add_to_aggregate(full_tuple *);
   void do_persistent_tuples(void);
   void process_persistent_tuple(full_tuple *, vm::tuple *);
	void process_consumed_local_tuples(void);
   void process_action_tuples(void);
   void process_incoming_tuples(void);
	void run_node(db::node *);
   bool sync(void);
   void setup(vm::predicate *, db::node*, const vm::derivation_count, const vm::depth_t);
   void cleanup(void);

   explicit state(sched::threads_sched *);
	explicit state(void);
   ~state(void);
};

}

#endif
