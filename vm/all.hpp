
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
   process::router *ROUTER;
   size_t NUM_THREADS;
   size_t NUM_NODES_PER_PROCESS;
   std::vector<sched::base*> SCHEDS;
	machine_arguments ARGS;
   std::vector<runtime::rstring::ptr> ARGUMENTS;

   inline void set_const(const const_id& id, const tuple_field d) { consts[id] = d; }

#define define_get_const(WHAT, TYPE, CODE) TYPE get_const_ ## WHAT (const const_id& id) { return CODE; }
	
	define_get_const(int, int_val, FIELD_INT(consts[id]))
	define_get_const(float, float_val, FIELD_FLOAT(consts[id]))
	define_get_const(ptr, ptr_val, FIELD_PTR(consts[id]));
	define_get_const(cons, runtime::cons*, FIELD_CONS(consts[id]))
	define_get_const(string, runtime::rstring*, FIELD_STRING(consts[id]))
	define_get_const(node, node_val, FIELD_NODE(consts[id]))
   define_get_const(struct, runtime::struct1*, FIELD_STRUCT(consts[id]))
	
#undef define_get_const

   inline tuple_field get_const(const const_id& id) { return consts[id]; }
	
#define define_set_const(WHAT, TYPE, CODE) void set_const_ ## WHAT (const const_id& id, const TYPE val) { CODE;}
	
	define_set_const(int, int_val&, SET_FIELD_INT(consts[id], val))
	define_set_const(float, float_val&, SET_FIELD_FLOAT(consts[id], val))
	define_set_const(node, node_val&, SET_FIELD_NODE(consts[id], val))
	define_set_const(ptr, ptr_val&, SET_FIELD_PTR(consts[id], val))
	define_set_const(string, runtime::rstring*, SET_FIELD_STRING(consts[id], val))
	
#undef define_set_const

	inline runtime::rstring::ptr get_argument(const argument_id id)
	{
		assert(id <= ARGUMENTS.size());
      runtime::rstring::ptr ret(ARGUMENTS[id-1]);

      if(ret == NULL)
         ARGUMENTS[id-1] = ret = runtime::rstring::make_string(ARGS[id-1]);
      return ret;
	}

   inline void set_arguments(const machine_arguments& args)
   {
      ARGS = args;
      ARGUMENTS.resize(args.size());
      for(size_t i(0); i < args.size(); ++i) {
         ARGUMENTS[i] = NULL;
      }
   }
	
   explicit all(void) {}

   virtual ~all(void)
   {
      for(size_t i(0); i < ARGUMENTS.size(); ++i) {
         if(ARGUMENTS[i] != NULL) {
            ARGUMENTS[i]->dec_refs();
         }
      }
   }
};

extern all *All;
extern program *theProgram;

}

#endif

