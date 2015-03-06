
#ifndef VM_ALL_HPP
#define VM_ALL_HPP

#include "vm/program.hpp"
#include "utils/utils.hpp"

#define MAX_CONSTS 32

// forward declaration
namespace db {
class database;
}
namespace sched {
class thread;
}

namespace process {

class machine;

class machine_error : public std::runtime_error {
   public:
   explicit machine_error(const std::string& msg) : std::runtime_error(msg) {}
};
}

namespace vm {

typedef std::vector<std::string> machine_arguments;

class all {
   private:
   tuple_field consts[MAX_CONSTS];

   public:
   vm::program* PROGRAM;

   db::database* DATABASE;
   process::machine* MACHINE;
   size_t NUM_THREADS;
   size_t NUM_THREADS_NEXT_UINT;
   size_t NUM_NODES_PER_PROCESS;
   std::vector<sched::thread*> SCHEDS;
   machine_arguments ARGS;
   std::vector<runtime::rstring::ptr> ARGUMENTS;
#ifdef INSTRUMENTATION
   std::vector<mem::pool*> THREAD_POOLS;
#endif

   inline void set_const(const const_id& id, const tuple_field d) {
      consts[id] = d;
   }

#define define_get_const(WHAT, TYPE, CODE) \
   TYPE get_const_##WHAT(const const_id& id) { return CODE; }

   define_get_const(int, int_val, FIELD_INT(consts[id]));
   define_get_const(float, float_val, FIELD_FLOAT(consts[id]));
   define_get_const(ptr, ptr_val, FIELD_PTR(consts[id]));
   define_get_const(cons, runtime::cons*, FIELD_CONS(consts[id]));
   define_get_const(string, runtime::rstring*, FIELD_STRING(consts[id]));
   define_get_const(node, node_val, FIELD_NODE(consts[id]));
   define_get_const(struct, runtime::struct1*, FIELD_STRUCT(consts[id]));
#undef define_get_const

   inline tuple_field get_const(const const_id& id) { return consts[id]; }

#define define_set_const(WHAT, TYPE, CODE) \
   void set_const_##WHAT(const const_id& id, const TYPE val) { CODE; }

   define_set_const(int, int_val&, SET_FIELD_INT(consts[id], val));
   define_set_const(float, float_val&, SET_FIELD_FLOAT(consts[id], val));
   define_set_const(node, node_val&, SET_FIELD_NODE(consts[id], val));
   define_set_const(ptr, ptr_val&, SET_FIELD_PTR(consts[id], val));
   define_set_const(string, runtime::rstring*,
                    SET_FIELD_STRING(consts[id], val));
#undef define_set_const

   inline runtime::rstring::ptr get_argument(const argument_id id) {
      assert(id <= ARGUMENTS.size());
      runtime::rstring::ptr ret(ARGUMENTS[id - 1]);

      if (ret == nullptr)
         ARGUMENTS[id - 1] = ret = runtime::rstring::make_string(ARGS[id - 1]);
      return ret;
   }

   inline void set_arguments(const machine_arguments& args) {
      ARGS = args;
      ARGUMENTS.resize(args.size());
      for (size_t i(0); i < args.size(); ++i) {
         ARGUMENTS[i] = nullptr;
      }
   }

   inline void check_arguments(const size_t needed) {
      if (ARGUMENTS.size() < needed)
         throw process::machine_error(std::string("this program requires ") +
                                      utils::to_string(needed) + " arguments");
   }

   explicit all(void) {}

   virtual ~all(void) {
      for (auto& elem : ARGUMENTS) {
         if (elem != nullptr) {
            elem->dec_refs();
         }
      }
   }
};

extern all* All;
extern program* theProgram;
}

#endif
