
#ifndef INSTR_HPP
#define INSTR_HPP

#include <cstring>
#include <ostream>
#include <stdexcept>

#include "vm/defs.hpp"
#include "vm/program.hpp"
#include "vm/external.hpp"

namespace vm {

typedef unsigned char instr_val;
typedef unsigned char reg_num;
typedef unsigned char offset_num;
typedef unsigned char callf_id;

const size_t MAGIC_SIZE = sizeof(uint64_t);
const uint32_t MAGIC1 = 0x646c656d;
const uint32_t MAGIC2 = 0x6c696620;

namespace instr {

const size_t field_size = 2;
const size_t iter_match_size = 2;
const size_t val_size = 1;
const size_t int_size = sizeof(int_val);
const size_t uint_size = sizeof(int_val);
const size_t float_size = sizeof(float_val);
const size_t node_size = sizeof(node_val);
const size_t string_size = sizeof(int_val);
const size_t ptr_size = sizeof(ptr_val);
const size_t bool_size = 1;
const size_t argument_size = 1;
const size_t reg_size = 0;
const size_t host_size = 0;
const size_t nil_size = 0;
const size_t tuple_size = 0;
const size_t index_size = 1;
const size_t jump_size = 4;
const size_t stack_val_size = sizeof(offset_num);
const size_t pcounter_val_size = 0;

const size_t SEND_BASE           = 3;
const size_t OP_BASE             = 5;
const size_t MOVE_BASE           = 3;
const size_t ITER_BASE           = 8;
const size_t ALLOC_BASE          = 3;
const size_t CALL_BASE           = 4;
const size_t IF_BASE             = 2 + jump_size;
const size_t MOVE_NIL_BASE       = 2;
const size_t TEST_NIL_BASE       = 3;
const size_t CONS_BASE           = 5;
const size_t HEAD_BASE           = 4;
const size_t TAIL_BASE           = 4;
const size_t NOT_BASE            = 3;
const size_t RETURN_BASE         = 1;
const size_t NEXT_BASE           = 1;
const size_t FLOAT_BASE          = 3;
const size_t SELECT_BASE         = 9;
const size_t RETURN_SELECT_BASE  = 5;
const size_t COLOCATED_BASE      = 4;
const size_t DELETE_BASE         = 3;
const size_t REMOVE_BASE         = 2;
const size_t RETURN_LINEAR_BASE  = 1;
const size_t RETURN_DERIVED_BASE = 1;
const size_t RESET_LINEAR_BASE   = 1 + jump_size;
const size_t END_LINEAR_BASE     = 1;
const size_t RULE_BASE           = 1 + uint_size;
const size_t RULE_DONE_BASE      = 1;
const size_t NEW_NODE_BASE       = 2;
const size_t NEW_AXIOMS_BASE     = 1 + jump_size;
const size_t SEND_DELAY_BASE     = 3 + uint_size;
const size_t PUSH_BASE           = 1;
const size_t POP_BASE            = 1;
const size_t PUSH_REGS_BASE      = 1;
const size_t POP_REGS_BASE       = 1;
const size_t CALLF_BASE          = 2;
const size_t CALLE_BASE          = 4;
const size_t STRUCT_VAL_BASE     = 4;
const size_t MAKE_STRUCT_BASE    = 3;

enum instr_type {
   RETURN_INSTR	      =  0x00,
   NEXT_INSTR		      =  0x01,
   ELSE_INSTR 		      =  0x02,
   TEST_NIL_INSTR	      =  0x03,
   CONS_INSTR		      =  0x04,
   HEAD_INSTR		      =  0x05,
   TAIL_INSTR		      =  0x06,
   NOT_INSTR		      =  0x07,
   SEND_INSTR 		      =  0x08,
   FLOAT_INSTR          =  0x09,
   SELECT_INSTR         =  0x0A,
   RETURN_SELECT_INSTR  =  0x0B,
   COLOCATED_INSTR      =  0x0C,
   DELETE_INSTR         =  0x0D,
   RESET_LINEAR_INSTR   =  0x0E,
   END_LINEAR_INSTR     =  0x0F,
   RULE_INSTR           =  0x10,
   RULE_DONE_INSTR      =  0x11,
   NEW_NODE_INSTR       =  0x13,
   NEW_AXIOMS_INSTR     =  0x14,
   SEND_DELAY_INSTR     =  0x15,
   PUSH_INSTR           =  0x16,
   POP_INSTR            =  0x17,
   PUSH_REGS_INSTR      =  0x18,
   POP_REGS_INSTR       =  0x19,
   CALLF_INSTR          =  0x1A,
   CALLE_INSTR          =  0x1B,
   STRUCT_VAL_INSTR     =  0x1C,
   MAKE_STRUCT_INSTR    =  0x1D,

   CALL_INSTR		      =  0x20,
   MOVE_INSTR		      =  0x30,
   ALLOC_INSTR		      =  0x40,
   IF_INSTR 		      =  0x60,
   MOVE_NIL_INSTR	      =  0x70,
   REMOVE_INSTR 	      =  0x80,
   ITER_INSTR		      =  0xA0,
   OP_INSTR			      =  0xC0,
   RETURN_LINEAR_INSTR  =  0xD0,
   RETURN_DERIVED_INSTR =  0xF0
};

enum instr_op {
   OP_NEQF       = 0x0,
   OP_NEQI       = 0x1,
   OP_EQF        = 0x2,
   OP_EQI        = 0x3,
   OP_LESSF      = 0x4,
   OP_LESSI      = 0x5,
   OP_LESSEQF    = 0x6,
   OP_LESSEQI    = 0x7,
   OP_GREATERF   = 0x8,
   OP_GREATERI   = 0x9,
   OP_GREATEREQF = 0xa,
   OP_GREATEREQI = 0xb,
   OP_MODF       = 0xc,
   OP_MODI       = 0xd,
   OP_PLUSF      = 0xe,
   OP_PLUSI      = 0xf,
   OP_MINUSF     = 0x10,
   OP_MINUSI     = 0x11,
   OP_TIMESF     = 0x12,
   OP_TIMESI     = 0x13,
   OP_DIVF       = 0x14,
   OP_DIVI       = 0x15,
   OP_NEQA       = 0x16,
   OP_EQA        = 0x17,
   OP_GREATERA   = 0x18,
   OP_ORB        = 0x19
};

std::string op_string(const instr_op op);

class malformed_instr_error : public std::runtime_error {
 public:
    explicit malformed_instr_error(const std::string& msg) :
         std::runtime_error(msg)
    {}
};

inline instr_type fetch(pcounter pc) { return (instr_type)*pc; }

/* val related functions */

enum val_code {
   VAL_TUPLE = 0x1f,
   VAL_PCOUNTER = 0x0A,
   VAL_PTR = 0x0B,
   VAL_BOOL = 0x0C
};

inline bool val_is_reg(const instr_val x) { return x & 0x20; }
inline bool val_is_tuple(const instr_val x) { return x == 0x1f; }
inline bool val_is_float(const instr_val x) { return x == 0x00; }
inline bool val_is_bool(const instr_val x) { return x == VAL_BOOL; }
inline bool val_is_int(const instr_val x) { return x == 0x01; }
inline bool val_is_field(const instr_val x) { return x == 0x02; }
inline bool val_is_host(const instr_val x) { return x == 0x03; }
inline bool val_is_nil(const instr_val x) { return x == 0x04; }
inline bool val_is_node(const instr_val x) { return x == 0x05; }
inline bool val_is_string(const instr_val x) { return x == 0x06; }
inline bool val_is_arg(const instr_val x) { return x == 0x07; }
inline bool val_is_const(const instr_val x) { return x == 0x08; }
inline bool val_is_stack(const instr_val x) { return x == 0x09; }
inline bool val_is_pcounter(const instr_val x) { return x == VAL_PCOUNTER; }
inline bool val_is_ptr(const instr_val x) { return x == VAL_PTR; }

inline bool_val pcounter_bool(const pcounter pc) { return *(utils::byte *)pc ? true : false; }
inline int_val pcounter_int(const pcounter pc) { return *(int_val *)pc; }
inline code_size_t pcounter_code_size(const pcounter pc) { return *(code_size_t *)pc; }
inline float_val pcounter_float(const pcounter pc) { return *(float_val *)pc; }
inline node_val pcounter_node(const pcounter pc) { return *(node_val *)pc; }
inline uint_val pcounter_uint(const pcounter pc) { return *(uint_val *)pc; }
inline argument_id pcounter_argument_id(const pcounter pc) { return (argument_id)*pc; }
inline const_id pcounter_const_id(const pcounter pc) { return pcounter_uint(pc); }
inline offset_num pcounter_offset_num(const pcounter pc) { return *pc; }
inline ptr_val pcounter_ptr(const pcounter pc) { return *(ptr_val *)pc; }

inline reg_num val_reg(const instr_val x) { return x & 0x1f; }
inline field_num val_field_num(const pcounter x) { return *x & 0xff; }
inline reg_num val_field_reg(const pcounter x) { return *(x + 1) & 0x1f; }

inline void pcounter_move_field(pcounter *pc) { *pc = *pc + field_size; }
inline void pcounter_move_bool(pcounter *pc) { *pc = *pc + bool_size; }
inline void pcounter_move_int(pcounter *pc) { *pc = *pc + int_size; }
inline void pcounter_move_float(pcounter *pc) { *pc = *pc + float_size; }
inline void pcounter_move_match(pcounter *pc) { *pc = *pc + iter_match_size; }
inline void pcounter_move_node(pcounter *pc) { *pc = *pc + node_size; }
inline void pcounter_move_uint(pcounter *pc) { *pc = *pc + uint_size; }
inline void pcounter_move_argument_id(pcounter *pc) { *pc = *pc + argument_size; }
inline void pcounter_move_const_id(pcounter *pc) { pcounter_move_uint(pc); }
inline void pcounter_move_offset_num(pcounter *pc) { *pc = *pc + stack_val_size; }
inline void pcounter_move_ptr(pcounter *pc) { *pc = *pc + ptr_size; }

/* common instruction functions */

inline code_offset_t jump_get(pcounter x, size_t off) { return pcounter_code_size(x + off); }
inline reg_num reg_get(pcounter x, size_t off) { return (reg_num)(*(x + off) & 0x1f); }
inline instr_val val_get(pcounter x, size_t off) { return (instr_val)(*(x + off) & 0x3f); }
inline predicate_id predicate_get(pcounter x, size_t off) { return (predicate_id)(*(x + off) & 0x7f); }
inline utils::byte byte_get(pcounter x, size_t off) { return *(utils::byte*)(x + off); }
inline field_num field_num_get(pcounter x, size_t off) { return (field_num)*(x + off); }

/* IF reg THEN ... ENDIF */

inline reg_num if_reg(pcounter pc) { return reg_get(pc, 1); }
inline code_offset_t if_jump(pcounter pc) { return jump_get(pc, 2); }

/* SEND a TO B */

inline reg_num send_msg(pcounter pc) { return reg_get(pc, 1); }
inline reg_num send_dest(pcounter pc) { return reg_get(pc, 2); }

/* FLOAT a TO b */

inline instr_val float_op(pcounter pc) { return val_get(pc, 1); }
inline instr_val float_dest(pcounter pc) { return val_get(pc, 2); }

/* OP a op b TO c */

inline instr_val op_arg1(pcounter pc) { return val_get(pc, 1); }
inline instr_val op_arg2(pcounter pc) { return val_get(pc, 2); }
inline instr_val op_dest(pcounter pc) { return val_get(pc, 3); }
inline instr_op op_op(pcounter pc) { return (instr_op)(*(pc + 4) & 0x1f); }

/* MOVE a TO b */

inline instr_val move_from(pcounter pc) { return val_get(pc, 1); }
inline instr_val move_to(pcounter pc) { return val_get(pc, 2); }
inline pcounter move_from_ptr(pcounter pc) { return pc + 1; }
inline pcounter move_to_ptr(pcounter pc) { return pc + 2; }

/* ITERATE pred MATCHING */

typedef pcounter iter_match;

inline predicate_id iter_predicate(pcounter pc) { return predicate_get(pc, 1); }
inline utils::byte iter_options(pcounter pc) { return byte_get(pc, 2); }
inline utils::byte iter_options_argument(pcounter pc) { return byte_get(pc, 3); }
inline code_offset_t iter_jump(pcounter pc) { return jump_get(pc, 4); }
inline bool iter_match_end(iter_match m) { return (*(m + 1) & 0xc0) == 0x40; }
inline bool iter_match_none(iter_match m) { return (*(m + 1) & 0xc0) == 0xc0; }
inline instr_val iter_match_val(iter_match m) { return val_get((pcounter)m, 1); }
inline field_num iter_match_field(iter_match m) { return (field_num)*m; }

inline bool iter_options_random(const utils::byte b) { return b & 0x01; }
inline bool iter_options_min(const utils::byte b) { return b & 0x04; }
inline field_num iter_options_min_arg(const utils::byte b) { return (field_num)b; }
inline bool iter_options_to_delete(const utils::byte b) { return b & 0x02; }

/* ALLOC pred to reg */

inline predicate_id alloc_predicate(pcounter pc) { return predicate_get(pc, 1); }
inline reg_num alloc_reg(pcounter pc) { return reg_get(pc, 2); }

/* CALL */

inline external_function_id call_extern_id(pcounter pc) { return (external_function_id)byte_get(pc, 1); }
inline size_t call_num_args(pcounter pc) { return (size_t)byte_get(pc, 2); }
inline reg_num call_dest(pcounter pc) { return reg_get(pc, 3); }
inline instr_val call_val(pcounter pc) { return val_get(pc, 0); }

/* CALLE (similar to CALL) */

inline external_function_id calle_extern_id(pcounter pc) { return (external_function_id)byte_get(pc, 1); }
inline size_t calle_num_args(pcounter pc) { return (size_t)byte_get(pc, 2); }
inline reg_num calle_dest(pcounter pc) { return reg_get(pc, 3); }
inline instr_val calle_val(pcounter pc) { return val_get(pc, 0); }

/* MOVE-NIL TO dst */

inline instr_val move_nil_dest(pcounter pc) { return val_get(pc, 1); }

/* TEST-NIL a TO b */

inline instr_val test_nil_op(pcounter pc) { return val_get(pc, 1); }
inline instr_val test_nil_dest(pcounter pc) { return val_get(pc, 2); }

/* CONS (a :: b) TO c */

inline size_t cons_type(pcounter pc) { return (size_t)byte_get(pc, 1); }
inline instr_val cons_head(pcounter pc) { return val_get(pc, 2); }
inline instr_val cons_tail(pcounter pc) { return val_get(pc, 3); }
inline instr_val cons_dest(pcounter pc) { return val_get(pc, 4); }

/* HEAD list TO a */

inline instr_val head_cons(pcounter pc) { return val_get(pc, 2); }
inline instr_val head_dest(pcounter pc) { return val_get(pc, 3); }

/* TAIL list TO a */

inline instr_val tail_cons(pcounter pc) { return val_get(pc, 2); }
inline instr_val tail_dest(pcounter pc) { return val_get(pc, 3); }

/* NOT a TO b */

inline instr_val not_op(pcounter pc) { return val_get(pc, 1); }
inline instr_val not_dest(pcounter pc) { return val_get(pc, 2); }

/* SELECT BY NODE ... */

inline code_size_t select_size(pcounter pc) { return pcounter_code_size(pc + 1); }
inline size_t select_hash_size(pcounter pc) { return (size_t)pcounter_code_size(pc + 1 + sizeof(code_size_t)); }
inline pcounter select_hash_start(pcounter pc) { return pc + SELECT_BASE; }
inline code_offset_t select_hash(pcounter hash_start, const node_val val) { return pcounter_code_size(hash_start + sizeof(code_size_t)*val); }
inline pcounter select_hash_code(pcounter hash_start, const size_t hash_size, const code_offset_t hashed) {
   return hash_start + hash_size*sizeof(code_size_t) + hashed - 1;
}

/* RETURN SELECT */

inline code_size_t return_select_jump(pcounter pc) { return pcounter_code_size(pc + 1); }

/* COLOCATED a b INTO c */

inline instr_val colocated_first(const pcounter pc) { return val_get(pc, 1); }
inline instr_val colocated_second(const pcounter pc) { return val_get(pc, 2); }
inline reg_num colocated_dest(const pcounter pc) { return reg_get(pc, 3); }

/* DELETE predicate */

inline predicate_id delete_predicate(const pcounter pc) { return predicate_get(pc, 1); }
inline size_t delete_num_args(pcounter pc) { return (size_t)byte_get(pc, 2); }
inline instr_val delete_val(pcounter pc) { return val_get(pc, index_size); }
inline field_num delete_index(pcounter pc) { return field_num_get(pc, 0); }

/* REMOVE */

inline reg_num remove_source(const pcounter pc) { return reg_get(pc, 1); }

/* RESET LINEAR */

inline code_offset_t reset_linear_jump(const pcounter pc) { return jump_get(pc, 1); }

/* RULE ID */

inline size_t rule_get_id(const pcounter pc) { return pcounter_uint(pc + 1); }

/* NEW NODE */

inline reg_num new_node_reg(const pcounter pc) { return reg_get(pc, 1); }

/* NEW AXIOMS */

inline code_offset_t new_axioms_jump(const pcounter pc) { return jump_get(pc, 1); }

/* SEND DELAY a TO b */

inline reg_num send_delay_msg(pcounter pc) { return reg_get(pc, 1); }
inline reg_num send_delay_dest(pcounter pc) { return reg_get(pc, 2); }
inline uint_val send_delay_time(pcounter pc) { return pcounter_uint(pc + 3); }

/* CALLF id */

inline callf_id callf_get_id(const pcounter pc) { return *(pc + 1); }

/* MAKE STRUCT size to */

inline size_t make_struct_type(pcounter pc) { return (size_t)byte_get(pc, 1); }
inline instr_val make_struct_to(pcounter pc) { return val_get(pc, 2); }

/* STRUCT VAL idx FROM a TO b */

inline size_t struct_val_idx(pcounter pc) { return (size_t)byte_get(pc, 1); }
inline instr_val struct_val_from(pcounter pc) { return val_get(pc, 2); }
inline instr_val struct_val_to(pcounter pc) { return val_get(pc, 3); }

/* advance function */

enum instr_argument_type {
   ARGUMENT_ANYTHING,
   ARGUMENT_ANYTHING_NOT_NIL,
   ARGUMENT_INT,
   ARGUMENT_WRITABLE,
   ARGUMENT_LIST,
   ARGUMENT_NON_LIST,
   ARGUMENT_STRUCT,
   ARGUMENT_BOOL,
   ARGUMENT_NODE
};

template <instr_argument_type type>
static inline size_t arg_size(const instr_val v);

#ifdef TEMPLATE_OPTIMIZERS
#define STATIC_INLINE static inline
#else
#define STATIC_INLINE
#endif

template <>
STATIC_INLINE size_t arg_size<ARGUMENT_ANYTHING>(const instr_val v)
{
   if(val_is_bool(v))
      return bool_size;
   else if(val_is_float(v))
      return float_size;
   else if(val_is_int(v))
      return int_size;
   else if(val_is_field(v))
      return field_size;
   else if(val_is_nil(v))
      return nil_size;
   else if(val_is_reg(v))
      return reg_size;
   else if(val_is_host(v))
      return host_size;
   else if(val_is_tuple(v))
      return tuple_size;
   else if(val_is_node(v))
      return node_size;
	else if(val_is_string(v))
		return string_size;
	else if(val_is_arg(v))
		return argument_size;
   else if(val_is_const(v))
		return int_size;
   else if(val_is_stack(v))
      return stack_val_size;
   else if(val_is_pcounter(v))
      return pcounter_val_size;
   else if(val_is_ptr(v))
      return ptr_size;
	else
      throw malformed_instr_error("invalid instruction argument value");
}

template <>
STATIC_INLINE size_t arg_size<ARGUMENT_ANYTHING_NOT_NIL>(const instr_val v)
{
   if(val_is_bool(v))
      return bool_size;
   else if(val_is_float(v))
      return float_size;
   else if(val_is_int(v))
      return int_size;
   else if(val_is_field(v))
      return field_size;
   else if(val_is_reg(v))
      return reg_size;
   else if(val_is_host(v))
      return host_size;
   else if(val_is_tuple(v))
      return tuple_size;
   else if(val_is_node(v))
      return node_size;
	else if(val_is_string(v))
		return string_size;
	else if(val_is_arg(v))
		return val_size;
	else if(val_is_const(v))
		return int_size;
   else if(val_is_stack(v))
      return stack_val_size;
   else if(val_is_pcounter(v))
      return pcounter_val_size;
   else if(val_is_ptr(v))
      return ptr_size;
   else {
      throw malformed_instr_error("invalid instruction argument value");
   }
}

template <>
STATIC_INLINE
size_t arg_size<ARGUMENT_INT>(const instr_val v)
{
   if(val_is_int(v))
      return int_size;
   else if(val_is_reg(v))
      return reg_size;
   else if(val_is_field(v))
      return field_size;
	else if(val_is_const(v))
		return int_size;
   else if(val_is_stack(v))
      return stack_val_size;
   else
      throw malformed_instr_error("invalid instruction int value");
}

template <>
STATIC_INLINE size_t arg_size<ARGUMENT_WRITABLE>(const instr_val v)
{
   if(val_is_reg(v))
      return reg_size;
   else if(val_is_field(v))
      return field_size;
	else if(val_is_const(v))
		return int_size;
   else if(val_is_stack(v))
      return stack_val_size;
   else if(val_is_pcounter(v))
      return pcounter_val_size;
   else
      throw malformed_instr_error("invalid instruction writable value");
}

template <>
STATIC_INLINE size_t arg_size<ARGUMENT_LIST>(const instr_val v)
{
   if(val_is_reg(v))
      return reg_size;
   else if(val_is_field(v))
      return field_size;
   else if(val_is_nil(v))
      return nil_size;
   else if(val_is_stack(v))
      return stack_val_size;
   else
      throw malformed_instr_error("invalid instruction list value");
}

template <>
STATIC_INLINE size_t arg_size<ARGUMENT_STRUCT>(const instr_val v)
{
   if(val_is_reg(v))
      return reg_size;
   else if(val_is_field(v))
      return field_size;
   else if(val_is_stack(v))
      return stack_val_size;
   else
      throw malformed_instr_error("invalid instruction struct value");
}

template <>
STATIC_INLINE size_t arg_size<ARGUMENT_NON_LIST>(const instr_val v)
{
   if(val_is_bool(v))
      return bool_size;
   else if(val_is_float(v))
      return float_size;
   else if(val_is_int(v))
      return int_size;
   else if(val_is_field(v))
      return field_size;
   else if(val_is_reg(v))
      return reg_size;
   else if(val_is_host(v))
      return host_size;
   else if(val_is_node(v))
      return node_size;
	else if(val_is_string(v))
		return string_size;
	else if(val_is_arg(v))
		return val_size;
	else if(val_is_const(v))
		return int_size;
   else if(val_is_stack(v))
      return stack_val_size;
   else if(val_is_pcounter(v))
      return pcounter_val_size;
   else if(val_is_ptr(v))
      return ptr_size;
   else
      throw malformed_instr_error("invalid instruction non-list value");
}

template <>
STATIC_INLINE size_t arg_size<ARGUMENT_BOOL>(const instr_val v)
{
   if(val_is_bool(v))
      return bool_size;
   else if(val_is_reg(v))
      return reg_size;
   else if(val_is_field(v))
      return field_size;
	else if(val_is_const(v))
		return int_size;
   else if(val_is_stack(v))
      return stack_val_size;
   else
      throw malformed_instr_error("invalid instruction bool value");
}

template <>
STATIC_INLINE size_t arg_size<ARGUMENT_NODE>(const instr_val v)
{
   if(val_is_reg(v))
      return reg_size;
   else if(val_is_field(v))
      return field_size;
   else if(val_is_host(v))
      return host_size;
   else if(val_is_node(v))
      return node_size;
	else if(val_is_const(v))
		return int_size;
   else if(val_is_stack(v))
      return stack_val_size;
   else
      throw malformed_instr_error("invalid instruction node value");
}

inline size_t
iter_matches_size(pcounter pc)
{
   if(iter_match_none(pc))
      return iter_match_size;
   
   size_t size = 0;
   size_t match_size;
   
   while(true) {
    
      match_size = iter_match_size;
      match_size += arg_size<ARGUMENT_ANYTHING>(iter_match_val(pc));
      size += match_size;
      
      if(iter_match_end(pc))
         return size;
      else
         pc += match_size;
   }
}

inline size_t
instr_call_args_size(pcounter arg, size_t num)
{
   size_t size;
   size_t total = 0;
   
   for(size_t i(0); i < num; ++i) {
      size = val_size + arg_size<ARGUMENT_ANYTHING>(call_val(arg));
      arg += size;
      total += size;
   }
   
   return total;
}

inline size_t
instr_delete_args_size(pcounter arg, size_t num)
{
   size_t size;
   size_t total = 0;
   
   for(size_t i(0); i < num; ++i) {
      size = index_size + val_size + arg_size<ARGUMENT_ANYTHING>(delete_val(arg));
      arg += size;
      total += size;
   }
   
   return total;
}

inline pcounter
advance(pcounter pc)
{
	switch(fetch(pc)) {
		case SEND_INSTR:
         return pc + SEND_BASE;
                   
      case FLOAT_INSTR:
         return pc + FLOAT_BASE
                   + arg_size<ARGUMENT_INT>(float_op(pc))
                   + arg_size<ARGUMENT_WRITABLE>(float_dest(pc));
                   
      case OP_INSTR:
         return pc + OP_BASE
                   + arg_size<ARGUMENT_ANYTHING>(op_arg1(pc))
                   + arg_size<ARGUMENT_ANYTHING>(op_arg2(pc))
                   + arg_size<ARGUMENT_WRITABLE>(op_dest(pc));
                   
      case MOVE_INSTR:
         return pc + MOVE_BASE
                   + arg_size<ARGUMENT_ANYTHING_NOT_NIL>(move_from(pc))
                   + arg_size<ARGUMENT_WRITABLE>(move_to(pc));
                   
      case ITER_INSTR:
         return pc + ITER_BASE
                   + iter_matches_size(pc + ITER_BASE);
                   
      case ALLOC_INSTR:
         return pc + ALLOC_BASE;
                   
      case CALL_INSTR:
         return pc + CALL_BASE
                   + instr_call_args_size(pc + CALL_BASE, call_num_args(pc));

      case CALLE_INSTR:
         return pc + CALLE_BASE
                   + instr_call_args_size(pc + CALLE_BASE, calle_num_args(pc));
                   
      case DELETE_INSTR:
         return pc + DELETE_BASE
                   + instr_delete_args_size(pc + DELETE_BASE, delete_num_args(pc));
                   
      case IF_INSTR:
         return pc + IF_BASE;
         
      case MOVE_NIL_INSTR:
         return pc + MOVE_NIL_BASE
                   + arg_size<ARGUMENT_WRITABLE>(move_nil_dest(pc));
                   
      case TEST_NIL_INSTR:
         return pc + TEST_NIL_BASE
                   + arg_size<ARGUMENT_LIST>(test_nil_op(pc))
                   + arg_size<ARGUMENT_WRITABLE>(test_nil_dest(pc));
      
      case CONS_INSTR:
         return pc + CONS_BASE
                   + arg_size<ARGUMENT_NON_LIST>(cons_head(pc))
                   + arg_size<ARGUMENT_LIST>(cons_tail(pc))
                   + arg_size<ARGUMENT_WRITABLE>(cons_dest(pc));
                   
      case HEAD_INSTR:
         return pc + HEAD_BASE
                   + arg_size<ARGUMENT_LIST>(head_cons(pc))
                   + arg_size<ARGUMENT_WRITABLE>(head_dest(pc));
                   
      case TAIL_INSTR:
         return pc + TAIL_BASE
                   + arg_size<ARGUMENT_LIST>(tail_cons(pc))
                   + arg_size<ARGUMENT_WRITABLE>(tail_dest(pc));
                   
      case NOT_INSTR:
         return pc + NOT_BASE
                   + arg_size<ARGUMENT_BOOL>(not_op(pc))
                   + arg_size<ARGUMENT_WRITABLE>(not_dest(pc));
                   
      case RETURN_INSTR:
         return pc + RETURN_BASE;

      case RETURN_LINEAR_INSTR:
         return pc + RETURN_LINEAR_BASE;
         
      case RETURN_DERIVED_INSTR:
         return pc + RETURN_DERIVED_BASE;
      
      case NEXT_INSTR:
         return pc + NEXT_BASE;
         
      case RETURN_SELECT_INSTR:
         return pc + RETURN_SELECT_BASE;

      case SELECT_INSTR:
         return pc + SELECT_BASE + select_hash_size(pc)*sizeof(code_size_t);
         
      case COLOCATED_INSTR:
         return pc + COLOCATED_BASE
                   + arg_size<ARGUMENT_NODE>(colocated_first(pc))
                   + arg_size<ARGUMENT_NODE>(colocated_second(pc));

      case REMOVE_INSTR:
         return pc + REMOVE_BASE;
         
      case RESET_LINEAR_INSTR:
         return pc + RESET_LINEAR_BASE;

      case END_LINEAR_INSTR:
         return pc + END_LINEAR_BASE;

      case RULE_INSTR:
         return pc + RULE_BASE;

      case RULE_DONE_INSTR:
         return pc + RULE_DONE_BASE;

      case NEW_NODE_INSTR:
         return pc + NEW_NODE_BASE;

      case NEW_AXIOMS_INSTR:
         return pc + new_axioms_jump(pc);
      
      case SEND_DELAY_INSTR:
         return pc + SEND_DELAY_BASE;

      case PUSH_INSTR:
         return pc + PUSH_BASE;

      case POP_INSTR:
         return pc + POP_BASE;

      case PUSH_REGS_INSTR:
         return pc + PUSH_REGS_BASE;

      case POP_REGS_INSTR:
         return pc + POP_REGS_BASE;

      case CALLF_INSTR:
         return pc + CALLF_BASE;

      case MAKE_STRUCT_INSTR:
         return pc + MAKE_STRUCT_BASE
                   + arg_size<ARGUMENT_WRITABLE>(make_struct_to(pc));

      case STRUCT_VAL_INSTR:
         return pc + STRUCT_VAL_BASE
                  + arg_size<ARGUMENT_STRUCT>(struct_val_from(pc))
                  + arg_size<ARGUMENT_WRITABLE>(struct_val_to(pc));
				
      case ELSE_INSTR:
      default:
         throw malformed_instr_error("unknown instruction code (advance)");
   }
}

/* instruction print functions */
std::string val_string(const instr_val, pcounter *);

}

/* byte code print functions */
pcounter instr_print(pcounter, const bool, const int, const program *, std::ostream&);
pcounter instr_print_simple(pcounter, const int, const program *, std::ostream&);
byte_code instrs_print(const byte_code, const code_size_t, const int, const program*, std::ostream&);

}

#endif
