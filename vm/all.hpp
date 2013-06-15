
#ifndef VM_ALL_HPP
#define VM_ALL_HPP

#include "vm/program.hpp"

#define MAX_CONSTS 32

// forward declaration
namespace db {
   class database;
}

namespace process {
   class remote;
   class machine;
   class router;
}

namespace sched {
	class base;
}

namespace vm
{

typedef std::vector<std::string> machine_arguments;

class all
{
   private:

	tuple_field consts[MAX_CONSTS];
	
   public:

   vm::program *PROGRAM;
   
   db::database *DATABASE;
   process::machine *MACHINE;
   //process::remote *REMOTE;
   process::router *ROUTER;
   size_t NUM_THREADS;
   size_t NUM_NODES_PER_PROCESS;
	machine_arguments ARGUMENTS;
   static const double TASK_STEALING_FACTOR = 0.2;
   std::vector<sched::base*> ALL_THREADS;

   inline void set_const(const const_id& id, const tuple_field d) { consts[id] = d; }

#define define_get_const(WHAT, TYPE, CODE) TYPE get_const_ ## WHAT (const const_id& id) { return CODE; }
	
	define_get_const(int, int_val, *(int_val*)(consts + id))
	define_get_const(float, float_val, *(float_val*)(consts + id))
	define_get_const(ptr, ptr_val, *(ptr_val*)(consts + id));
	define_get_const(int_list, runtime::int_list*, (runtime::int_list*)get_const_ptr(id))
	define_get_const(float_list, runtime::float_list*, (runtime::float_list*)get_const_ptr(id))
	define_get_const(node_list, runtime::node_list*, (runtime::node_list*)get_const_ptr(id))
	define_get_const(string, runtime::rstring::ptr, (runtime::rstring::ptr)get_const_ptr(id))
	define_get_const(node, node_val, *(node_val*)(consts + id))
	
#undef define_get_const

   inline tuple_field get_const(const const_id& id) { return consts[id]; }
	
#define define_set_const(WHAT, TYPE, CODE) void set_const_ ## WHAT (const const_id& id, const TYPE val) { CODE;}
	
	define_set_const(int, int_val&, *(int_val*)(consts + id) = val)
	define_set_const(float, float_val&, *(float_val*)(consts + id) = val)
	define_set_const(node, node_val&, *(node_val*)(consts +id) = val)
	define_set_const(ptr, ptr_val&, *(ptr_val*)(consts + id) = val);
	define_set_const(string, runtime::rstring::ptr, set_const_ptr(id, (ptr_val)val));
	
#undef define_set_const

	inline runtime::rstring::ptr get_argument(const argument_id id)
	{
		assert(id <= ARGUMENTS.size());
		return runtime::rstring::make_string(ARGUMENTS[id-1]);
	}
	
   explicit all(void) {}
};

}

#endif

