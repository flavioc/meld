
#ifndef VM_STATE_HPP
#define VM_STATE_HPP

#include <list>

#include "vm/tuple.hpp"
#include "db/node.hpp"
#include "vm/program.hpp"
#include "vm/instr.hpp"
#include "db/trie.hpp"

// forward declaration
namespace process {
   class process;
   class remote;
   class machine;
   class router;
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
   typedef all_val reg;
   reg regs[NUM_REGS];
   db::tuple_trie_leaf *saved_leafs[NUM_REGS];
   
   std::list<runtime::float_list*, mem::allocator<runtime::float_list*> > free_float_list;
   std::list<runtime::int_list*, mem::allocator<runtime::int_list*> > free_int_list;
   std::list<runtime::node_list*, mem::allocator<runtime::node_list*> > free_node_list;
	std::list<runtime::rstring::ptr, mem::allocator<runtime::rstring::ptr> > free_rstring;
   
   typedef std::pair<vm::tuple *, vm::ref_count> pair_linear;
   typedef std::list<pair_linear> list_linear;

	static reg consts[MAX_CONSTS];

public:
	
#define define_get_const(WHAT, TYPE, CODE) static TYPE get_const_ ## WHAT (const const_id& id) { return CODE; }
	
	define_get_const(int, int_val, *(int_val*)(consts + id))
	define_get_const(float, float_val, *(float_val*)(consts + id))
	define_get_const(ptr, ptr_val, *(ptr_val*)(consts + id));
	define_get_const(int_list, runtime::int_list*, (runtime::int_list*)get_const_ptr(id))
	define_get_const(float_list, runtime::float_list*, (runtime::float_list*)get_const_ptr(id))
	define_get_const(node_list, runtime::node_list*, (runtime::node_list*)get_const_ptr(id))
	define_get_const(string, runtime::rstring::ptr, (runtime::rstring::ptr)get_const_ptr(id))
	
#undef define_get_const
	
#define define_set_const(WHAT, TYPE, CODE) static void set_const_ ## WHAT (const const_id& id, const TYPE val) { CODE;}
	
	define_set_const(int, int_val&, *(int_val*)(consts + id) = val)
	define_set_const(float, float_val&, *(float_val*)(consts + id) = val)

#undef define_set_const
	
   vm::tuple *tuple;
   db::tuple_trie_leaf *tuple_leaf;
   db::node *node;
   ref_count count;
   process::process *proc;
   bool is_linear;
   list_linear used_linear_tuples;
   
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
   
#define define_get(WHAT, RET, BODY) \
   inline RET get_ ## WHAT (const reg_num& num) const { BODY; }
   
   define_get(reg, reg, return regs[num]);
   define_get(int, int_val, return *(int_val*)(regs + num));
   define_get(float, float_val, return *(float_val*)(regs + num));
   define_get(ptr, ptr_val, return *(ptr_val*)(regs + num));
   define_get(bool, bool_val, return get_int(num) ? true : false);
	define_get(string, runtime::rstring::ptr, return (runtime::rstring::ptr)get_ptr(num););
   define_get(int_list, runtime::int_list*, return (runtime::int_list*)get_ptr(num));
   define_get(float_list, runtime::float_list*, return (runtime::float_list*)get_ptr(num));
   define_get(node_list, runtime::node_list*, return (runtime::node_list*)get_ptr(num));
   define_get(tuple, vm::tuple*, return (vm::tuple*)get_ptr(num));
   define_get(node, vm::node_val, return *(node_val *)(regs + num));
   
#undef define_get

#define define_set(WHAT, ARG, BODY) \
   inline void set_ ## WHAT (const reg_num& num, ARG val) { BODY; };
   
   define_set(float, const float_val&, *(float_val*)(regs + num) = val);
   define_set(int, const int_val&, *(int_val*)(regs + num) = val);
   define_set(ptr, const ptr_val&, *(ptr_val*)(regs + num) = val);
   define_set(bool, const bool_val&, set_int(num, val ? 1 : 0));
	define_set(string, const runtime::rstring::ptr, set_ptr(num, (ptr_val)val));
   define_set(int_list, runtime::int_list*, set_ptr(num, (ptr_val)val));
   define_set(float_list, runtime::float_list*, set_ptr(num, (ptr_val)val));
   define_set(node_list, runtime::node_list*, set_ptr(num, (ptr_val)val));
   define_set(tuple, vm::tuple*, set_ptr(num, (ptr_val)val));
   define_set(node, const node_val, *(node_val*)(regs + num) = val);
   
#undef define_set
   
   inline void set_nil(const reg_num& num) { set_ptr(num, null_ptr_val); }
   
   inline void set_leaf(const reg_num& num, db::tuple_trie_leaf* leaf) { saved_leafs[num] = leaf; }
   inline db::tuple_trie_leaf* get_leaf(const reg_num& num) const { return saved_leafs[num]; }
   
   inline void copy_reg(const reg_num& reg_from, const reg_num& reg_to) {
      regs[reg_to] = regs[reg_from];
   }

	void copy_reg2const(const reg_num&, const const_id&);
   
   inline void add_float_list(runtime::float_list *ls) { free_float_list.push_back(ls); }
   inline void add_int_list(runtime::int_list *ls) { free_int_list.push_back(ls); }
   inline void add_node_list(runtime::node_list *ls) { free_node_list.push_back(ls); }
	inline void add_string(runtime::rstring::ptr str) { free_rstring.push_back(str); }
   void purge_runtime_objects(void);
   
   void setup(vm::tuple*, db::node*, const ref_count);
   void cleanup(void);
   bool linear_tuple_can_be_used(vm::tuple *, const vm::ref_count) const;
   void using_new_linear_tuple(vm::tuple *);
   void no_longer_using_linear_tuple(vm::tuple *);
   
	static inline runtime::rstring::ptr get_argument(const argument_id id)
	{
		assert(id <= ARGUMENTS.size());
		return runtime::rstring::make_string(ARGUMENTS[id-1]);
	}
	
   explicit state(process::process *_proc):
      proc(_proc)
   {
   }

	explicit state(void):
		proc(NULL)
	{
	}
};

}

#endif
