
#ifndef STATE_HPP
#define STATE_HPP

#include "vm/tuple.hpp"
#include "db/node.hpp"
#include "db/database.hpp"
#include "vm/program.hpp"
#include "vm/instr.hpp"

namespace vm {

class state
{
private:
   
   static const size_t NUM_REGS = 32;
   typedef unsigned long int reg;
   
public:
   
   reg regs[NUM_REGS];
   vm::tuple *tuple;
   db::node *node;
   ref_count count;
   program *prog;
   db::database *db;
   
   inline reg get_reg(const reg_num& num) const { return regs[num]; }
   
   inline const int_val get_int(const reg_num& num) const {
      return *(int_val*)(regs + num);
   }
   
   inline const float_val get_float(const reg_num& num) const {
      return *(float_val*)(regs + num);
   }
   
   inline const addr_val get_addr(const reg_num& num) const {
      return *(addr_val*)(regs + num);
   }
   
   inline const bool_val get_bool(const reg_num& num) const {
      return get_int(num) ? true : false;
   }
   
   inline runtime::int_list* get_int_list(const reg_num& num) const {
      return (runtime::int_list*)get_addr(num);
   }
   
   inline runtime::float_list* get_float_list(const reg_num& num) const {
      return (runtime::float_list*)get_addr(num);
   }
   
   inline runtime::addr_list* get_addr_list(const reg_num& num) const {
      return (runtime::addr_list*)get_addr(num);
   }
   
   inline vm::tuple* get_tuple(const reg_num& num) const {
      return (vm::tuple*)regs[num];
   }
   
   inline db::node* get_node(const reg_num& num) const {
      return (db::node*)regs[num];
   }
   
   inline void set_float(const reg_num& num, const float_val& val) {
      *(float_val*)(regs + num) = val;
   }
   
   inline void set_int(const reg_num& num, const int_val& val) {
      *(int_val*)(regs + num) = val;
   }
   
   inline void set_addr(const reg_num& num, const addr_val& val) {
      *(addr_val*)(regs + num) = val;
   }
   
   inline void set_bool(const reg_num& num, const bool_val& val) {
      set_int(num, val ? 1 : 0);
   }
   
   inline void set_nil(const reg_num& num) { set_addr(num, NULL); }
   
   inline void set_int_list(const reg_num& num, runtime::int_list* val) {
      set_addr(num, val);
   }
   
   inline void set_float_list(const reg_num& num, runtime::float_list* val) {
      set_addr(num, val);
   }
   
   inline void set_addr_list(const reg_num& num, runtime::addr_list* val) {
      set_addr(num, val);
   }
   
   inline void set_tuple(const reg_num& num, vm::tuple *tuple) { set_addr(num, (addr_val)tuple); }
   
   inline void copy_reg(const reg_num& reg_from, const reg_num& reg_to) {
      regs[reg_to] = regs[reg_from];
   }
   
   explicit state(void) {}
};

}

#endif
