
#ifndef STATE_HPP
#define STATE_HPP

#include "vm/tuple.hpp"
#include "db/node.hpp"
#include "db/database.hpp"
#include "vm/program.hpp"
#include "vm/instr.hpp"
#include "process/exec.hpp"

namespace vm {

class state
{
private:
   
   static const size_t NUM_REGS = 32;
   typedef unsigned long int reg;
   
   reg regs[NUM_REGS];
   
public:
   
   vm::tuple *tuple;
   db::node *node;
   ref_count count;
   
   static program *PROGRAM;
   static db::database *DATABASE;
   static process::machine *MACHINE;
   
#define define_get(WHAT, RET, BODY) \
   inline RET get_ ## WHAT (const reg_num& num) const { BODY; }
   
   define_get(reg, reg, return regs[num]);
   define_get(int, const int_val, return *(int_val*)(regs + num));
   define_get(float, const float_val, return *(float_val*)(regs + num));
   define_get(addr, const addr_val, return *(addr_val*)(regs + num));
   define_get(bool, const bool_val, return get_int(num) ? true : false);
   define_get(int_list, runtime::int_list*, return (runtime::int_list*)get_addr(num));
   define_get(float_list, runtime::float_list*, return (runtime::float_list*)get_addr(num));
   define_get(addr_list, runtime::addr_list*, return (runtime::addr_list*)get_addr(num));
   define_get(tuple, vm::tuple*, return (vm::tuple*)get_addr(num));
   define_get(node, db::node*, return (db::node*)get_addr(num));
   
#undef define_get

#define define_set(WHAT, ARG, BODY) \
   inline void set_ ## WHAT (const reg_num& num, ARG val) { BODY; };
   
   define_set(float, const float_val&, *(float_val*)(regs + num) = val);
   define_set(int, const int_val&, *(int_val*)(regs + num) = val);
   define_set(addr, const addr_val&, *(addr_val*)(regs + num) = val);
   define_set(bool, const bool_val&, set_int(num, val ? 1 : 0));
   define_set(int_list, runtime::int_list*, set_addr(num, val));
   define_set(float_list, runtime::float_list*, set_addr(num, val));
   define_set(addr_list, runtime::addr_list*, set_addr(num, val));
   define_set(tuple, vm::tuple*, set_addr(num, (addr_val)val));
   
#undef define_set
   
   inline void set_nil(const reg_num& num) { set_addr(num, NULL); }
   
   inline void copy_reg(const reg_num& reg_from, const reg_num& reg_to) {
      regs[reg_to] = regs[reg_from];
   }
   
   explicit state(void) {}
};

}

#endif
