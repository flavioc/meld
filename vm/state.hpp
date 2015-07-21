
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
#include "vm/buffer.hpp"

// forward declaration
namespace sched {
class thread;
}

namespace vm {

struct state {
   private:
   std::list<std::pair<runtime::cons *, vm::list_type*>, mem::allocator<std::pair<runtime::cons *, vm::list_type*>>> free_cons;
   std::list<runtime::rstring::ptr, mem::allocator<runtime::rstring::ptr>>
       free_rstring;
   std::list<std::pair<runtime::struct1 *, vm::struct_type *>,
             mem::allocator<std::pair<runtime::struct1 *, vm::struct_type *>>>
       free_struct1;
   std::list<std::pair<runtime::array *, vm::type *>,
             mem::allocator<std::pair<runtime::array *, vm::type *>>>
       free_array;
   std::list<std::pair<runtime::set *, vm::type *>,
             mem::allocator<std::pair<runtime::set *, vm::type *>>>
       free_set;

   void purge_runtime_objects();
   full_tuple *search_for_negative_tuple(vm::full_tuple_list*, full_tuple *);

   void indexing_state_machine(db::node *);

public:

   db::node *node;
   sched::thread *sched;
   vm::rule_matcher *matcher;
   vm::counter *match_counter{nullptr};
   vm::depth_t depth;
   derivation_direction direction;
#ifndef COMPILED
   using reg = tuple_field;

   reg regs[NUM_REGS];
   vm::predicate *preds[NUM_REGS];
   vm::bitmap_static<1> updated_map;
   vm::bitmap_static<1> tuple_regs;
   call_stack stack;
   bool is_linear;
   utils::randgen randgen;
   size_t current_rule;

   bool running_rule;
   bool hash_removes;
   std::unordered_set<utils::byte *, utils::pointer_hash<utils::byte>,
                      std::equal_to<utils::byte *>,
                      mem::allocator<utils::byte *>> allocated_match_objects;

   inline bool tuple_is_used(const vm::tuple *tpl, const reg_num r)
   {
      for(auto it(tuple_regs.begin(NUM_REGS)); !it.end(); ++it) {
         const size_t x(*it);
         if(x >= (size_t)r)
            return false;
         if(get_tuple(x) == tpl) {
            return true;
         }
      }
      return false;
   }

#define define_get(WHAT, RET, BODY) \
   inline RET get_##WHAT(const reg_num &num) const { BODY; }

   define_get(reg, reg, return regs[num]);
   define_get(int, int_val, return FIELD_INT(regs[num]));
   define_get(float, float_val, return FIELD_FLOAT(regs[num]));
   define_get(ptr, ptr_val, return FIELD_PTR(regs[num]));
   define_get(bool, bool_val, return FIELD_BOOL(regs[num]));
   define_get(string, runtime::rstring::ptr, return FIELD_STRING(regs[num]););
   define_get(cons, runtime::cons *, return FIELD_CONS(regs[num]));
   define_get(tuple, vm::tuple *, return (vm::tuple *)get_ptr(num));
   define_get(node, vm::node_val, return FIELD_NODE(regs[num]));
   define_get(struct, runtime::struct1 *, return FIELD_STRUCT(regs[num]));
   define_get(array, runtime::array *, return FIELD_ARRAY(regs[num]));
   define_get(thread, thread_val, return FIELD_THREAD(regs[num]));

#undef define_get

#define define_set(WHAT, ARG, BODY) \
   inline void set_##WHAT(const reg_num &num, ARG val) { BODY; };

   define_set(float, const float_val &, SET_FIELD_FLOAT(regs[num], val));
   define_set(int, const int_val &, SET_FIELD_INT(regs[num], val));
   define_set(ptr, const ptr_val &, SET_FIELD_PTR(regs[num], val));
   define_set(bool, const bool_val &, SET_FIELD_BOOL(regs[num], val));
   define_set(string, const runtime::rstring::ptr,
              SET_FIELD_STRING(regs[num], val));
   define_set(cons, runtime::cons *, SET_FIELD_CONS(regs[num], val));
   define_set(tuple, vm::tuple *, set_ptr(num, (ptr_val)val));
   define_set(node, const node_val, SET_FIELD_NODE(regs[num], val));
   define_set(thread, const thread_val, SET_FIELD_THREAD(regs[num], val));
   define_set(struct, runtime::struct1 *, SET_FIELD_STRUCT(regs[num], val));
   define_set(array, runtime::array *, SET_FIELD_ARRAY(regs[num], val));
   define_set(set, runtime::set *, SET_FIELD_SET(regs[num], val));

#undef define_set

   inline void set_reg(const reg_num &num, const reg val) { regs[num] = val; }
   inline void set_nil(const reg_num &num) { set_ptr(num, null_ptr_val); }
   inline reg get_reg(const reg_num &num) { return regs[num]; }

   inline void copy_reg(const reg_num &reg_from, const reg_num &reg_to) {
      regs[reg_to] = regs[reg_from];
   }

   void copy_reg2const(const reg_num &, const const_id &);

#endif
#ifdef FACT_BUFFERING
   vm::buffer facts_to_send;
#endif
#ifdef COORDINATION_BUFFERING
   using map_set_priority = std::unordered_map<
       const db::node *, vm::priority_t, db::node_hash,
       std::equal_to<const db::node *>,
       mem::allocator<std::pair<const db::node *const, vm::priority_t>>>;
   map_set_priority set_priorities;
#endif
#ifdef DEBUG_MODE
   bool print_instrs{false};
#endif

   size_t linear_facts_generated;
   size_t persistent_facts_generated;
   size_t linear_facts_consumed;
   // resets previous counters.
   void reset_counters();

#ifdef INSTRUMENTATION
   std::atomic<size_t> instr_facts_consumed{0};
   std::atomic<size_t> instr_facts_derived{0};
   std::atomic<size_t> instr_rules_run{0};
#endif
   bool generated_facts;
   // generated linear facts
#ifdef COMPILED
   tuple_list generated[COMPILED_NUM_LINEAR];
#else
   tuple_list *generated{nullptr};
#endif
   vm::full_tuple_list node_persistent_tuples;
   vm::full_tuple_list thread_persistent_tuples;

   inline tuple_list* get_generated(const vm::predicate_id p)
   {
      assert(p < vm::theProgram->num_linear_predicates());
      return generated + p;
   }

   inline void add_generated(vm::tuple *tpl, const vm::predicate *pred)
   {
      tuple_list *ls(get_generated(pred->get_linear_id()));
      ls->push_back(tpl);
      generated_facts = true;
      linear_facts_generated++;
   }

   inline void add_node_persistent_fact(vm::full_tuple *stpl)
   {
      node_persistent_tuples.push_back(stpl);
      persistent_facts_generated++;
   }

   inline void add_thread_persistent_fact(vm::full_tuple *stpl)
   {
      thread_persistent_tuples.push_back(stpl);
      persistent_facts_generated++;
   }

   candidate_gc_nodes gc_nodes;
#ifdef CORE_STATISTICS
   core_statistics stat;
#endif

   inline void add_cons(runtime::cons *ls, vm::list_type *t) {
      ls->inc_refs();
      free_cons.push_back(std::make_pair(ls, t));
   }
   inline void add_array(runtime::array *x, vm::type *t) {
      x->inc_refs();
      free_array.push_back(std::make_pair(x, t));
   }
   inline void add_set(runtime::set *x, vm::type *t) {
      x->inc_refs();
      free_set.push_back(std::make_pair(x, t));
   }
   inline void add_string(runtime::rstring::ptr str) {
      str->inc_refs();
      free_rstring.push_back(str);
   }
   inline void add_struct(runtime::struct1 *s, vm::struct_type *t) {
      s->inc_refs();
      free_struct1.push_back(std::make_pair(s, t));
   }

   void add_to_aggregate(db::node *, vm::full_tuple_list *, full_tuple *);
   inline void do_persistent_tuples(db::node *, vm::full_tuple_list*);
   inline void process_persistent_tuple(db::node *, full_tuple *, vm::tuple *);
   inline void process_consumed_local_tuples(void);
   inline void process_action_tuples(db::node *);
   inline void process_incoming_tuples(db::node *);
   void run_node(db::node *);
   inline bool sync(db::node *);
   void setup(vm::predicate *, const vm::derivation_direction,
              const vm::depth_t);
   void cleanup(void);

   explicit state(sched::thread *);
   explicit state(void);
   ~state(void);
};
}

#endif
