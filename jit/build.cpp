
#include <jit/jit-dump.h>
#include <jit/jit-intrinsic.h>
#include <iostream>

#include "jit/build.hpp"
#include "thread/threads.hpp"
#include "machine.hpp"
#include "vm/tuple.hpp"

using namespace vm;
using namespace std;
using namespace instr;

namespace jit
{

static jit_type_t reg_type;
static jit_type_t reg_ptr_type;
static jit_type_t set_priority0_type;
static jit_type_t send0_type;
static predicate *preds[NUM_REGS];
static bool created_based_types = false;

#include "vm/helpers.cpp"

jit_function_t
jit_create_function(pcounter pc, pcounter until)
{
   jit_context_t ctx = jit_context_create();

   jit_context_build_start(ctx);

   jit_type_t signature;
   jit_type_t params[3];
   params[0] = reg_ptr_type; /* regs */
   params[1] = jit_type_void_ptr; /* state */
   params[2] = jit_type_void_ptr; /* node */
   signature = jit_type_create_signature(jit_abi_cdecl, jit_type_void, params, sizeof(params)/sizeof(jit_type_t), 0);
   jit_function_t function = jit_function_create(ctx, signature);
   jit_value_t regs(jit_value_get_param(function, 0));
   jit_value_t state(jit_value_get_param(function, 1));
   jit_value_t node(jit_value_get_param(function, 2));
   assert(regs);

	for (; pc < until; ) {
      //cout << instr_name(fetch(pc)) << endl;
      switch(fetch(pc)) {
         case RULE_INSTR:
         case RULE_DONE_INSTR:
            break;
         case MVINTFIELD_INSTR:
            {
               const reg_num reg(val_field_reg(pc + instr_size + int_size));
               const field_num f(val_field_num(pc + instr_size + int_size));
               const int_val i(pcounter_int(pc + instr_size));
               // get tuple value
               jit_value_t toval(jit_value_create_nint_constant(function, jit_type_int, i));
               jit_value_t tuplev(jit_insn_load_relative(function, regs, reg * sizeof(vm::tuple_field), reg_type));
               jit_insn_store_relative(function, tuplev, sizeof(vm::tuple) + f * sizeof(vm::tuple_field), toval);
               break;
            }
         case MVREGFIELD_INSTR:
            {
               const reg_num reg(pcounter_reg(pc + instr_size));
               const reg_num treg(val_field_reg(pc + instr_size + reg_val_size));
               const field_num f(val_field_num(pc + instr_size + reg_val_size));
               size_t off(treg * sizeof(vm::tuple_field));
               jit_value_t tuplev(jit_insn_load_relative(function, regs, off, reg_type));
               jit_value_t regv(jit_insn_load_relative(function, regs, reg * sizeof(vm::tuple_field), reg_type));

               off = sizeof(vm::tuple) + f * sizeof(vm::tuple_field);
               jit_insn_store_relative(function, tuplev, off, regv);
               break;
            }

         case MVFIELDREG_INSTR:
            {
               const reg_num treg(val_field_reg(pc + instr_size));
               const field_num f(val_field_num(pc + instr_size));
               const reg_num dest(pcounter_reg(pc + instr_size + field_size));
               size_t off(treg * sizeof(vm::tuple_field));
               jit_value_t tuplev(jit_insn_load_relative(function, regs, off, reg_type));
               off = sizeof(vm::tuple) + f * sizeof(vm::tuple_field);
               jit_value_t field(jit_insn_load_relative(function, tuplev, off, reg_type));

               jit_insn_store_relative(function, regs, dest * sizeof(vm::tuple_field), field);
               break;
            }
         case INTPLUS_INSTR:
            {
               const reg_num a(pcounter_reg(pc + instr_size));
               const reg_num b(pcounter_reg(pc + instr_size + reg_val_size));
               const reg_num c(pcounter_reg(pc + instr_size + 2 * reg_val_size));
               jit_value_t va(jit_insn_load_relative(function, regs, sizeof(vm::tuple_field) * a, jit_type_int));
               jit_value_t vb(jit_insn_load_relative(function, regs, sizeof(vm::tuple_field) * b, jit_type_int));
               jit_value_t sum(jit_insn_add(function, va, vb));
               jit_insn_store_relative(function, regs, c * sizeof(vm::tuple_field), sum);
               break;
            }
         case INTLESSEREQUAL_INSTR:
            {
               const reg_num a(pcounter_reg(pc + instr_size));
               const reg_num b(pcounter_reg(pc + instr_size + reg_val_size));
               const reg_num c(pcounter_reg(pc + instr_size + 2 * reg_val_size));
               jit_value_t va(jit_insn_load_relative(function, regs, sizeof(vm::tuple_field) * a, jit_type_int));
               jit_value_t vb(jit_insn_load_relative(function, regs, sizeof(vm::tuple_field) * b, jit_type_int));
               jit_value_t res(jit_insn_le(function, va, vb));
               jit_insn_store_relative(function, regs, c * sizeof(vm::tuple_field), res);
               break;
            }
         case FLOAT_INSTR:
            {
               const reg_num src(pcounter_reg(pc + instr_size));
               const reg_num dst(pcounter_reg(pc + instr_size + reg_val_size));
               jit_value_t vsrc(jit_insn_load_relative(function, regs, sizeof(vm::tuple_field) * src, jit_type_int));
               jit_intrinsic_descr_t desc = {jit_type_float64, nullptr, jit_type_int, nullptr};
               jit_value_t res(jit_insn_call_intrinsic(function, "int2float", (void*)jit_int_to_float64, &desc, vsrc, nullptr));
               jit_insn_store_relative(function, regs, dst * sizeof(vm::tuple_field), res);
               break;
            }
         case SET_PRIORITY_INSTR:
            {
               const reg_num prio_reg(pcounter_reg(pc + instr_size));
               const reg_num node_reg(pcounter_reg(pc + instr_size + reg_val_size));
               jit_value_t prio(jit_insn_load_relative(function, regs, sizeof(vm::tuple_field) * prio_reg, jit_type_float64));
               jit_value_t node(jit_insn_load_relative(function, regs, sizeof(vm::tuple_field) * node_reg, jit_type_void_ptr));
               jit_value_t args[] = {node, prio, state};
               jit_insn_call_native(function, "set_priority0", (void*)execute_set_priority0, set_priority0_type, args, 3, 0);
               break;
            }
         case SEND_INSTR:
            {
               const reg_num msg(send_msg(pc));
               const reg_num dest(send_dest(pc));
               jit_value_t dest_val(jit_insn_load_relative(function, regs,
                        sizeof(vm::tuple_field) * dest, jit_type_void_ptr));
               predicate *pred(preds[(size_t)msg]);
               if(!pred) {
                  abort();
               }
               jit_value_t tpl(jit_insn_load_relative(function, regs,
                        sizeof(vm::tuple_field) * msg, jit_type_void_ptr));
               jit_value_t predv(jit_value_create_nint_constant(function, jit_type_void_ptr, (jit_nint)pred));
               jit_value_t args[] = {node, dest_val, tpl, predv, state};
               jit_insn_call_native(function, "execute_send0", (void*)execute_send0, send0_type, args, 5, 0);
               break;
            }
         case MVHOSTREG_INSTR:
         case MVADDRREG_INSTR:
         case ADDRNOTEQUAL_INSTR:
         case MVFLOATREG_INSTR:
         case MVPTRREG_INSTR:
         case INTLESSER_INSTR:
            break;
      }
      pc = advance(pc);
   }

//   jit_dump_function(stdout, function, "fun");
   jit_context_build_end(ctx);
   jit_function_compile(function);
//   jit_dump_function(stdout, function, "fun");
   //jit_context_destroy(ctx);
   return function;
}

void
jit_compile(byte_code code, const size_t len)
{
   if(!created_based_types) {
      created_based_types = true;

      jit_init();

      // create union type for registers
      jit_type_t available_reg_types[] = {jit_type_int, jit_type_float64, jit_type_void_ptr, 
         jit_type_uint};
      reg_type = jit_type_create_union(available_reg_types, sizeof(available_reg_types)/sizeof(jit_type_t), 0);
      reg_ptr_type = jit_type_create_pointer(reg_type, 1);
      assert(jit_type_get_size(jit_type_int) == sizeof(int_val));
      assert(jit_type_get_size(jit_type_uint) == sizeof(uint_val));
      assert(jit_type_get_size(jit_type_float64) == sizeof(float_val));
      assert(jit_type_get_size(jit_type_void_ptr) == sizeof(ptr_val));

      jit_type_t spparams[3];
      spparams[0] = jit_type_void_ptr; /* node */
      spparams[1] = jit_type_float64; /* prio */
      spparams[2] = jit_type_void_ptr; /* state */
      set_priority0_type = jit_type_create_signature(jit_abi_cdecl, jit_type_void, spparams, sizeof(spparams)/sizeof(jit_type_t), 0);
      {
         jit_type_t params[5];
         params[0] = jit_type_void_ptr; /* node */
         params[1] = jit_type_void_ptr; /* dest_val */
         params[2] = jit_type_void_ptr; /* tuple */
         params[3] = jit_type_void_ptr; /* pred */
         params[4] = jit_type_void_ptr; /* state */
         send0_type = jit_type_create_signature(jit_abi_cdecl, jit_type_void, params, sizeof(params)/sizeof(jit_type_t), 0);
      }
   }

   pcounter pc(code);
   pcounter until = code + len;
   pcounter start;
   bool has_start(false);

	for (; pc < until; ) {
      //cout << instr_name(fetch(pc)) << endl;
      switch(fetch(pc)) {
         case RULE_INSTR:
         case RULE_DONE_INSTR:
         case MVFIELDREG_INSTR:
         case INTPLUS_INSTR:
         case FLOAT_INSTR:
         case SET_PRIORITY_INSTR:
         case MVREGFIELD_INSTR:
         case MVINTFIELD_INSTR:
         case INTLESSEREQUAL_INSTR:
         case SEND_INSTR:
            if(!has_start) {
               has_start = true;
               start = pc;
            }
            break;
         case OPERS_ITER_INSTR:
         case OLINEAR_ITER_INSTR:
         case ORLINEAR_ITER_INSTR:
         case PERS_ITER_INSTR:
         case TPERS_ITER_INSTR:
         case LINEAR_ITER_INSTR:
         case TLINEAR_ITER_INSTR:
         case RLINEAR_ITER_INSTR:

            {
               // fall through
               predicate *pred(theProgram->get_predicate(iter_predicate(pc)));
               const reg_num reg(iter_reg(pc));
               preds[(size_t)reg] = pred;
               goto stop;
            }

         case ALLOC_INSTR:
            {
               predicate *pred(theProgram->get_predicate(alloc_predicate(pc)));
               const reg_num reg(alloc_reg(pc));

               preds[reg] = pred;
               goto stop;
            }

         case MVHOSTREG_INSTR:
         case MVADDRREG_INSTR:
         case ADDRNOTEQUAL_INSTR:

         case MVFLOATREG_INSTR:
         case MVPTRREG_INSTR:
         case INTLESSER_INSTR:
         case REMOVE_INSTR:
         case SET_PRIORITYH_INSTR:
         case UPDATE_INSTR:

         case RETURN_DERIVED_INSTR:
         case IF_INSTR:
         case SELECT_INSTR:
         case RETURN_INSTR:
         case NEW_AXIOMS_INSTR:
         case RETURN_SELECT_INSTR:
         case NEXT_INSTR:
         default:
stop:
            if(has_start) {
               size_t size(pc - start);
               has_start = false;
               if(size >= JIT_BASE) {
                  //cout << "adding code size " << size << "\n";
                  jit_function_t f(jit_create_function(start, pc));
                  *start = JIT_INSTR;
                  *(code_size_t*)(start + instr_size) = size;
                  *(void**)(start + instr_size + jump_size) = jit_function_to_closure(f);
               }
            }
            break;
            cerr << "Unrecognized instruction " << instr_name(fetch(pc)) << "\n";
            abort();
      }
#if 0
      if(has_start)
         cout << "has ";
      instr_print_simple(pc, 0, theProgram, cout);
#endif
      pc = advance(pc);
   }
}

}
