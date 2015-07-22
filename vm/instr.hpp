
#ifndef INSTR_HPP
#define INSTR_HPP

#include <cstring>
#include <ostream>
#include <stdexcept>

#include "vm/defs.hpp"
#include "vm/program.hpp"
#include "vm/external.hpp"

namespace vm {

// if using mem addresses for nodes.
extern bool USING_MEM_ADDRESSES;

typedef unsigned char instr_val;
typedef unsigned char reg_num;
typedef unsigned char offset_num;
typedef unsigned char callf_id;

const size_t MAGIC_SIZE = sizeof(uint64_t);
const uint32_t MAGIC1 = 0x646c656d;
const uint32_t MAGIC2 = 0x6c696620;

namespace instr {

const size_t type_size = sizeof(utils::byte);
const size_t predicate_size = sizeof(utils::byte);
const size_t extern_id_size = sizeof(utils::byte);
const size_t instr_size = sizeof(utils::byte);
const size_t field_size = 2 * sizeof(utils::byte);
const size_t iter_match_size = 2 * sizeof(utils::byte);
const size_t val_size = sizeof(utils::byte);
const size_t int_size = sizeof(int_val);
const size_t uint_size = sizeof(int_val);
const size_t float_size = sizeof(float_val);
const size_t node_size = sizeof(node_val);
const size_t string_size = sizeof(int_val);
const size_t ptr_size = sizeof(ptr_val);
const size_t const_id_size = uint_size;
const size_t bool_size = sizeof(utils::byte);
const size_t argument_size = 1;
const size_t reg_size = 0;
const size_t reg_val_size = 1;
const size_t host_size = 0;
const size_t nil_size = 0;
const size_t non_nil_size = 0;
const size_t any_size = 0;
const size_t tuple_size = 0;
const size_t index_size = 1;
const size_t jump_size = 4;
const size_t count_size = sizeof(utils::byte);
const size_t stack_val_size = sizeof(offset_num);
const size_t pcounter_val_size = 0;
const size_t operation_size = instr_size + 3 * reg_val_size;
const size_t call_size =
    instr_size + extern_id_size + reg_val_size + type_size + bool_size;
const size_t iter_options_size = 2 * sizeof(utils::byte);

const size_t SEND_BASE = instr_size + 2 * reg_val_size;
const size_t THREAD_SEND_BASE = SEND_BASE;
const size_t OP_BASE = instr_size + 4;
const size_t ITER_BASE0 = instr_size + ptr_size + predicate_size +
                          reg_val_size + bool_size + 2 * jump_size;
const size_t ITER_BASE = ITER_BASE0 + count_size;
const size_t OITER_BASE = ITER_BASE + iter_options_size;
const size_t PERS_ITER_BASE = ITER_BASE;
const size_t TPERS_ITER_BASE = ITER_BASE;
const size_t LINEAR_ITER_BASE = ITER_BASE;
const size_t TLINEAR_ITER_BASE = ITER_BASE;
const size_t TRLINEAR_ITER_BASE = ITER_BASE;
const size_t RLINEAR_ITER_BASE = ITER_BASE;
const size_t OPERS_ITER_BASE = OITER_BASE;
const size_t OLINEAR_ITER_BASE = OITER_BASE;
const size_t ORLINEAR_ITER_BASE = OITER_BASE;
const size_t ALLOC_BASE = instr_size + predicate_size + 2 * reg_val_size;
const size_t CALL_BASE = call_size + count_size;
const size_t IF_BASE = instr_size + 1 + jump_size;
const size_t TESTNIL_BASE = instr_size + reg_val_size + reg_val_size;
const size_t HEAD_BASE = instr_size + 3;
const size_t NOT_BASE = instr_size + 2 * reg_val_size;
const size_t RETURN_BASE = instr_size;
const size_t NEXT_BASE = instr_size;
const size_t FLOAT_BASE = instr_size + 2 * reg_val_size;
const size_t SELECT_BASE = instr_size + 8;
const size_t RETURN_SELECT_BASE = instr_size + 4;
const size_t REMOVE_BASE = instr_size + reg_val_size;
const size_t RETURN_LINEAR_BASE = instr_size;
const size_t RETURN_DERIVED_BASE = instr_size;
const size_t RESET_LINEAR_BASE = instr_size + jump_size;
const size_t END_LINEAR_BASE = instr_size;
const size_t RULE_BASE = instr_size + uint_size;
const size_t RULE_DONE_BASE = instr_size;
const size_t NEW_NODE_BASE = instr_size + reg_val_size;
const size_t NEW_AXIOMS_BASE = instr_size + jump_size;
const size_t SEND_DELAY_BASE = instr_size + 2 + uint_size;
const size_t PUSH_BASE = instr_size;
const size_t POP_BASE = instr_size;
const size_t PUSH_REGS_BASE = instr_size;
const size_t POP_REGS_BASE = instr_size;
const size_t CALLF_BASE = instr_size + 1;
const size_t CALLE_BASE = call_size + count_size;
const size_t MAKE_STRUCTR_BASE = instr_size + type_size + reg_val_size;
const size_t MVINTFIELD_BASE = instr_size + int_size + field_size;
const size_t MVINTREG_BASE = instr_size + int_size + reg_val_size;
const size_t MVTYPEREG_BASE = MVINTREG_BASE;
const size_t MVTHREADIDREG_BASE = instr_size + reg_val_size;
const size_t MVTHREADIDFIELD_BASE = instr_size + field_size;
const size_t MVFIELDFIELD_BASE = instr_size + field_size + field_size;
const size_t MVFIELDREG_BASE = instr_size + field_size + reg_val_size;
const size_t MVPTRREG_BASE = instr_size + ptr_size + reg_val_size;
const size_t MVNILFIELD_BASE = instr_size + field_size;
const size_t MVNILREG_BASE = instr_size + reg_val_size;
const size_t MVREGFIELD_BASE = instr_size + reg_val_size + field_size;
const size_t MVHOSTFIELD_BASE = instr_size + field_size;
const size_t MVREGCONST_BASE = instr_size + reg_val_size + const_id_size;
const size_t MVCONSTFIELD_BASE = instr_size + const_id_size + field_size;
const size_t MVADDRFIELD_BASE = instr_size + node_size + field_size;
const size_t MVFLOATFIELD_BASE = instr_size + float_size + field_size;
const size_t MVFLOATREG_BASE = instr_size + float_size + reg_val_size;
const size_t MVINTCONST_BASE = instr_size + int_size + const_id_size;
const size_t MVWORLDFIELD_BASE = instr_size + field_size;
const size_t MVSTACKPCOUNTER_BASE = instr_size + stack_val_size;
const size_t MVPCOUNTERSTACK_BASE = instr_size + stack_val_size;
const size_t MVSTACKREG_BASE = instr_size + stack_val_size + reg_val_size;
const size_t MVSTACKFIELD_BASE = instr_size + stack_val_size + field_size;
const size_t MVREGSTACK_BASE = instr_size + reg_val_size + stack_val_size;
const size_t MVADDRREG_BASE = instr_size + node_size + reg_val_size;
const size_t MVHOSTREG_BASE = instr_size + reg_val_size;
const size_t MVREGREG_BASE = instr_size + 2 * reg_val_size;
const size_t MVARGREG_BASE = instr_size + argument_size + reg_val_size;
const size_t HEADRR_BASE = instr_size + 2 * reg_val_size;
const size_t HEADFR_BASE = instr_size + field_size + reg_val_size;
const size_t HEADFF_BASE = instr_size + 2 * field_size;
const size_t HEADRF_BASE = instr_size * reg_val_size + field_size;
const size_t TAILRR_BASE = HEADRR_BASE;
const size_t TAILFR_BASE = HEADFR_BASE;
const size_t TAILFF_BASE = HEADFF_BASE;
const size_t TAILRF_BASE = HEADRF_BASE;
const size_t MVWORLDREG_BASE = instr_size + reg_val_size;
const size_t MVCONSTREG_BASE = instr_size + const_id_size + reg_val_size;
const size_t CONSRRR_BASE =
    instr_size + type_size + 3 * reg_val_size + bool_size;
const size_t CONSRFF_BASE = instr_size + reg_val_size + 2 * field_size;
const size_t CONSFRF_BASE = instr_size + field_size + reg_val_size + field_size;
const size_t CONSFFR_BASE =
    instr_size + 2 * field_size + reg_val_size + bool_size;
const size_t CONSRRF_BASE = instr_size + 2 * reg_val_size + field_size;
const size_t CONSRFR_BASE =
    instr_size + reg_val_size + field_size + reg_val_size + bool_size;
const size_t CONSFRR_BASE =
    instr_size + type_size + field_size + 2 * reg_val_size + bool_size;
const size_t CONSFFF_BASE = instr_size + 3 * field_size;
const size_t CALL0_BASE = call_size;
const size_t CALL1_BASE = call_size + reg_val_size;
const size_t CALL2_BASE = call_size + 2 * reg_val_size;
const size_t CALL3_BASE = call_size + 3 * reg_val_size;
const size_t MVINTSTACK_BASE = instr_size + int_size + stack_val_size;
const size_t PUSHN_BASE = instr_size + count_size;
const size_t MAKE_STRUCTF_BASE = instr_size + field_size;
const size_t STRUCT_VALRR_BASE = instr_size + count_size + 2 * reg_val_size;
const size_t STRUCT_VALFR_BASE =
    instr_size + count_size + field_size + reg_val_size;
const size_t STRUCT_VALRF_BASE = STRUCT_VALFR_BASE;
const size_t STRUCT_VALRFR_BASE = STRUCT_VALRF_BASE;
const size_t STRUCT_VALFF_BASE = instr_size + count_size + 2 * field_size;
const size_t STRUCT_VALFFR_BASE = STRUCT_VALFF_BASE;
const size_t MVFLOATSTACK_BASE = instr_size + float_size + stack_val_size;
const size_t ADDLINEAR_BASE = instr_size + reg_val_size;
const size_t ADDPERS_BASE = ADDLINEAR_BASE;
const size_t ADDTPERS_BASE = ADDLINEAR_BASE;
const size_t RUNACTION_BASE = ADDLINEAR_BASE;
const size_t ENQUEUE_LINEAR_BASE = ADDLINEAR_BASE;
const size_t UPDATE_BASE = instr_size + reg_val_size;
const size_t SET_PRIORITY_BASE = instr_size + 2 * reg_val_size;
const size_t SET_PRIORITYH_BASE = instr_size + reg_val_size;
const size_t ADD_PRIORITY_BASE = instr_size + 2 * reg_val_size;
const size_t ADD_PRIORITYH_BASE = instr_size + reg_val_size;
const size_t SET_DEFPRIOH_BASE = instr_size + reg_val_size;
const size_t SET_DEFPRIO_BASE = instr_size + 2 * reg_val_size;
const size_t SET_STATICH_BASE = instr_size;
const size_t SET_STATIC_BASE = instr_size + reg_val_size;
const size_t SET_MOVINGH_BASE = instr_size;
const size_t SET_MOVING_BASE = instr_size + reg_val_size;
const size_t SET_AFFINITYH_BASE = instr_size + reg_val_size;
const size_t SET_AFFINITY_BASE = instr_size + reg_val_size + reg_val_size;
const size_t CPU_STATIC_BASE = instr_size + 2 * reg_val_size;
const size_t MVCPUSREG_BASE = instr_size + reg_val_size;
const size_t SET_CPUH_BASE = instr_size + reg_val_size;
const size_t IS_STATIC_BASE = instr_size + 2 * reg_val_size;
const size_t IS_MOVING_BASE = instr_size + 2 * reg_val_size;
const size_t STOP_PROG_BASE = instr_size;
const size_t CPU_ID_BASE = instr_size + 2 * reg_val_size;
const size_t NODE_PRIORITY_BASE = instr_size + 2 * reg_val_size;
const size_t REM_PRIORITY_BASE = instr_size + reg_val_size;
const size_t REM_PRIORITYH_BASE = instr_size;
const size_t FACTS_PROVED_BASE = instr_size + 2 * reg_val_size;
const size_t FACTS_CONSUMED_BASE = instr_size + 2 * reg_val_size;
const size_t SCHEDULE_NEXT_BASE = instr_size + reg_val_size;
const size_t IF_ELSE_BASE = instr_size + reg_val_size + 2 * jump_size;
const size_t JUMP_BASE = instr_size + jump_size;
const size_t FABS_BASE = instr_size + 2 * reg_val_size;
const size_t REMOTE_UPDATE_BASE =
    instr_size + reg_val_size + 2 * predicate_size + 2 * count_size;
const size_t MARK_RULE_BASE = instr_size + uint_size;
const size_t LITERAL_CONS_BASE = instr_size + reg_val_size + type_size + jump_size;
const size_t NODE_TYPE_BASE = instr_size + reg_val_size + uint_size;

enum instr_type {
   RETURN_INSTR = 0x00,
   NEXT_INSTR = 0x01,
   PERS_ITER_INSTR = 0x02,
   TESTNIL_INSTR = 0x03,
   OPERS_ITER_INSTR = 0x04,
   LINEAR_ITER_INSTR = 0x05,
   RLINEAR_ITER_INSTR = 0x06,
   NOT_INSTR = 0x07,
   SEND_INSTR = 0x08,
   FLOAT_INSTR = 0x09,
   SELECT_INSTR = 0x0A,
   RETURN_SELECT_INSTR = 0x0B,
   OLINEAR_ITER_INSTR = 0x0C,
   RESET_LINEAR_INSTR = 0x0E,
   END_LINEAR_INSTR = 0x0F,
   RULE_INSTR = 0x10,
   RULE_DONE_INSTR = 0x11,
   ORLINEAR_ITER_INSTR = 0x12,
   NEW_NODE_INSTR = 0x13,
   NEW_AXIOMS_INSTR = 0x14,
   SEND_DELAY_INSTR = 0x15,
   PUSH_INSTR = 0x16,
   POP_INSTR = 0x17,
   PUSH_REGS_INSTR = 0x18,
   POP_REGS_INSTR = 0x19,
   CALLF_INSTR = 0x1A,
   CALLE_INSTR = 0x1B,
   SET_PRIORITY_INSTR = 0x1C,
   MAKE_STRUCTR_INSTR = 0x1D,
   MVINTFIELD_INSTR = 0x1E,
   MVINTREG_INSTR = 0x1F,
   CALL_INSTR = 0x20,
   MVFIELDFIELD_INSTR = 0x21,
   MVFIELDREG_INSTR = 0x22,
   MVPTRREG_INSTR = 0x23,
   MVNILREG_INSTR = 0x24,
   MVFIELDFIELDR_INSTR = 0x25,
   MVREGFIELD_INSTR = 0x26,
   MVREGFIELDR_INSTR = 0x27,
   MVHOSTFIELD_INSTR = 0x28,
   MVREGCONST_INSTR = 0x29,
   MVCONSTFIELD_INSTR = 0x2A,
   MVCONSTFIELDR_INSTR = 0x2B,
   MVADDRFIELD_INSTR = 0x2C,
   MVFLOATFIELD_INSTR = 0x2D,
   MVFLOATREG_INSTR = 0x2E,
   MVINTCONST_INSTR = 0x2F,
   SET_PRIORITYH_INSTR = 0x30,
   MVWORLDFIELD_INSTR = 0x31,
   MVSTACKPCOUNTER_INSTR = 0x32,
   MVPCOUNTERSTACK_INSTR = 0x33,
   MVSTACKREG_INSTR = 0x34,
   MVREGSTACK_INSTR = 0x35,
   MVADDRREG_INSTR = 0x36,
   MVHOSTREG_INSTR = 0x37,
   ADDRNOTEQUAL_INSTR = 0x38,
   ADDREQUAL_INSTR = 0x39,
   INTMINUS_INSTR = 0x3A,
   INTEQUAL_INSTR = 0x3B,
   INTNOTEQUAL_INSTR = 0x3C,
   INTPLUS_INSTR = 0x3D,
   INTLESSER_INSTR = 0x3E,
   INTGREATEREQUAL_INSTR = 0x3F,
   ALLOC_INSTR = 0x40,
   BOOLOR_INSTR = 0x41,
   INTLESSEREQUAL_INSTR = 0x42,
   INTGREATER_INSTR = 0x43,
   INTMUL_INSTR = 0x44,
   INTDIV_INSTR = 0x45,
   FLOATPLUS_INSTR = 0x46,
   FLOATMINUS_INSTR = 0x47,
   FLOATMUL_INSTR = 0x48,
   FLOATDIV_INSTR = 0x49,
   FLOATEQUAL_INSTR = 0x4A,
   FLOATNOTEQUAL_INSTR = 0x4B,
   FLOATLESSER_INSTR = 0x4C,
   FLOATLESSEREQUAL_INSTR = 0x4D,
   FLOATGREATER_INSTR = 0x4E,
   FLOATGREATEREQUAL_INSTR = 0x4F,
   MVREGREG_INSTR = 0x50,
   BOOLEQUAL_INSTR = 0x51,
   BOOLNOTEQUAL_INSTR = 0x52,
   HEADRR_INSTR = 0x53,
   HEADFR_INSTR = 0x54,
   HEADFF_INSTR = 0x55,
   HEADRF_INSTR = 0x56,
   HEADFFR_INSTR = 0x57,
   HEADRFR_INSTR = 0x58,
   TAILRR_INSTR = 0x59,
   TAILFR_INSTR = 0x5A,
   TAILFF_INSTR = 0x5B,
   TAILRF_INSTR = 0x5C,
   MVWORLDREG_INSTR = 0x5D,
   MVCONSTREG_INSTR = 0x5E,
   CONSRRR_INSTR = 0x5F,
   IF_INSTR = 0x60,
   CONSRFF_INSTR = 0x61,
   CONSFRF_INSTR = 0x62,
   CONSFFR_INSTR = 0x63,
   CONSRRF_INSTR = 0x64,
   CONSRFR_INSTR = 0x65,
   CONSFRR_INSTR = 0x66,
   CONSFFF_INSTR = 0x67,
   CALL0_INSTR = 0x68,
   CALL1_INSTR = 0x69,
   CALL2_INSTR = 0x6A,
   CALL3_INSTR = 0x6B,
   MVINTSTACK_INSTR = 0x6C,
   PUSHN_INSTR = 0x6D,
   MAKE_STRUCTF_INSTR = 0x6E,
   STRUCT_VALRR_INSTR = 0x6F,
   MVNILFIELD_INSTR = 0x70,
   STRUCT_VALFR_INSTR = 0x71,
   STRUCT_VALRF_INSTR = 0x72,
   STRUCT_VALRFR_INSTR = 0x73,
   STRUCT_VALFF_INSTR = 0x74,
   STRUCT_VALFFR_INSTR = 0x75,
   MVFLOATSTACK_INSTR = 0x76,
   ADDLINEAR_INSTR = 0x77,
   ADDPERS_INSTR = 0x78,
   RUNACTION_INSTR = 0x79,
   ENQUEUE_LINEAR_INSTR = 0x7A,
   UPDATE_INSTR = 0x7B,
   MVARGREG_INSTR = 0x7C,
   INTMOD_INSTR = 0x7D,
   CPU_ID_INSTR = 0x7E,
   NODE_PRIORITY_INSTR = 0x7F,
   REMOVE_INSTR = 0x80,
   IF_ELSE_INSTR = 0x81,
   JUMP_INSTR = 0x82,
   MVSTACKFIELD_INSTR = 0x83,
   REMOTE_UPDATE_INSTR = 0x84,
   ADD_PRIORITY_INSTR = 0xA0,
   ADD_PRIORITYH_INSTR = 0xA1,
   STOP_PROG_INSTR = 0xA2,
   SET_DEFPRIOH_INSTR = 0xA3,
   SET_DEFPRIO_INSTR = 0xA4,
   SET_STATICH_INSTR = 0xA5,
   SET_STATIC_INSTR = 0xA6,
   SET_MOVINGH_INSTR = 0xA7,
   SET_MOVING_INSTR = 0xA8,
   SET_AFFINITYH_INSTR = 0xA9,
   SET_AFFINITY_INSTR = 0xAA,
   CPU_STATIC_INSTR = 0xAB,
   MVCPUSREG_INSTR = 0xAC,
   SET_CPUH_INSTR = 0xAD,
   IS_STATIC_INSTR = 0xAE,
   IS_MOVING_INSTR = 0xAF,
   FACTS_PROVED_INSTR = 0xB0,
   REM_PRIORITY_INSTR = 0xB1,
   REM_PRIORITYH_INSTR = 0xB2,
   FACTS_CONSUMED_INSTR = 0xB3,
   TLINEAR_ITER_INSTR = 0xB4,
   ADDTPERS_INSTR = 0xB5,
   SCHEDULE_NEXT_INSTR = 0xB6,
   TPERS_ITER_INSTR = 0xB7,
   FABS_INSTR = 0xB8,
   MVTYPEREG_INSTR = 0xB9,
   MVTHREADIDREG_INSTR = 0xBA,
   MVTHREADIDFIELD_INSTR = 0xBB,
   THREAD_SEND_INSTR = 0xBC,
   TRLINEAR_ITER_INSTR = 0xBD,
   LITERAL_CONS_INSTR = 0xBE,
   NODE_TYPE_INSTR = 0xBF,
   RETURN_LINEAR_INSTR = 0xD0,
   MARK_RULE_INSTR = 0xD1,
   RETURN_DERIVED_INSTR = 0xF0
};

class malformed_instr_error : public std::runtime_error {
   public:
   explicit malformed_instr_error(const std::string &msg)
       : std::runtime_error(msg) {}
};

inline instr_type fetch(pcounter pc) { return (instr_type)*pc; }

/* val related functions */

enum val_code {
   VAL_TUPLE = 0x1f,
   VAL_PCOUNTER = 0x0A,
   VAL_PTR = 0x0B,
   VAL_BOOL = 0x0C,
   VAL_NIL = 0x04,
   VAL_NON_NIL = 0x0D,
   VAL_LIST = 0xE,
   VAL_ANY = 0x0F
};

inline bool val_is_reg(const instr_val x) { return x & 0x20; }
inline bool val_is_float(const instr_val x) { return x == 0x00; }
inline bool val_is_bool(const instr_val x) { return x == VAL_BOOL; }
inline bool val_is_int(const instr_val x) { return x == 0x01; }
inline bool val_is_field(const instr_val x) { return x == 0x02; }
inline bool val_is_host(const instr_val x) { return x == 0x03; }
inline bool val_is_nil(const instr_val x) { return x == VAL_NIL; }
inline bool val_is_non_nil(const instr_val x) { return x == VAL_NON_NIL; }
inline bool val_is_any(const instr_val x) { return x == VAL_ANY; }
inline bool val_is_list(const instr_val x) { return x == VAL_LIST; }
inline bool val_is_node(const instr_val x) { return x == 0x05; }
inline bool val_is_string(const instr_val x) { return x == 0x06; }
inline bool val_is_arg(const instr_val x) { return x == 0x07; }
inline bool val_is_const(const instr_val x) { return x == 0x08; }
inline bool val_is_stack(const instr_val x) { return x == 0x09; }
inline bool val_is_pcounter(const instr_val x) { return x == VAL_PCOUNTER; }
inline bool val_is_ptr(const instr_val x) { return x == VAL_PTR; }

/* value functions for registers and fields */
inline reg_num val_reg(const instr_val x) { return x & 0x1f; }
inline field_num val_field_num(const pcounter x) { return *x & 0xff; }
inline reg_num val_field_reg(const pcounter x) { return *(x + 1) & 0x1f; }

/* common instruction functions */
inline code_size_t pcounter_code_size(const pcounter pc) {
   return *(code_size_t *)pc;
}
inline code_offset_t jump_get(pcounter x, size_t off) {
   return pcounter_code_size(x + off);
}
inline reg_num reg_get(pcounter x, size_t off) {
   return (reg_num)(*(x + off) & 0x1f);
}
inline instr_val val_get(pcounter x, size_t off) {
   return (instr_val)(*(x + off) & 0x3f);
}
inline predicate_id predicate_get(pcounter x, size_t off) {
   return (predicate_id)(*(x + off) & 0x7f);
}
inline utils::byte byte_get(pcounter x, size_t off) {
   return *(utils::byte *)(x + off);
}
inline field_num field_num_get(pcounter x, size_t off) {
   return (field_num) * (x + off);
}

inline bool_val pcounter_bool(const pcounter pc) {
   return *(utils::byte *)pc ? true : false;
}
inline int_val pcounter_int(const pcounter pc) { return *(int_val *)pc; }
inline float_val pcounter_float(const pcounter pc) { return *(float_val *)pc; }
inline node_val pcounter_node(const pcounter pc) { return *(node_val *)pc; }
inline uint_val pcounter_uint(const pcounter pc) { return *(uint_val *)pc; }
inline argument_id pcounter_argument_id(const pcounter pc) {
   return (argument_id)*pc;
}
inline const_id pcounter_const_id(const pcounter pc) {
   return pcounter_uint(pc);
}
inline offset_num pcounter_offset_num(const pcounter pc) { return *pc; }
inline ptr_val pcounter_ptr(const pcounter pc) { return *(ptr_val *)pc; }
inline reg_num pcounter_reg(const pcounter x) { return *x; }
inline offset_num pcounter_stack(const pcounter pc) {
   return pcounter_offset_num(pc);
}

inline void pcounter_set_node(pcounter pc, const node_val n) {
   *(node_val *)pc = n;
}

inline void pcounter_move_byte(pcounter *pc) { *pc = *pc + 1; }
inline void pcounter_move_field(pcounter *pc) { *pc = *pc + field_size; }
inline void pcounter_move_bool(pcounter *pc) { *pc = *pc + bool_size; }
inline void pcounter_move_int(pcounter *pc) { *pc = *pc + int_size; }
inline void pcounter_move_float(pcounter *pc) { *pc = *pc + float_size; }
inline void pcounter_move_match(pcounter *pc) { *pc = *pc + iter_match_size; }
inline void pcounter_move_node(pcounter *pc) { *pc = *pc + node_size; }
inline void pcounter_move_uint(pcounter *pc) { *pc = *pc + uint_size; }
inline void pcounter_move_argument_id(pcounter *pc) {
   *pc = *pc + argument_size;
}
inline void pcounter_move_const_id(pcounter *pc) { pcounter_move_uint(pc); }
inline void pcounter_move_offset_num(pcounter *pc) {
   *pc = *pc + stack_val_size;
}
inline void pcounter_move_ptr(pcounter *pc) { *pc = *pc + ptr_size; }

/* IF reg THEN ... ENDIF */

inline reg_num if_reg(pcounter pc) { return pcounter_reg(pc + instr_size); }
inline code_offset_t if_jump(pcounter pc) {
   return jump_get(pc, instr_size + reg_val_size);
}

/* IF reg THEN ... ELSE ... ENDIF */

inline code_offset_t if_else_jump_else(pcounter pc) {
   return jump_get(pc, instr_size + reg_val_size);
}
inline code_offset_t if_else_jump(pcounter pc) {
   return jump_get(pc, instr_size + reg_val_size + jump_size);
}

/* SEND a TO B */

inline reg_num send_msg(pcounter pc) { return pcounter_reg(pc + instr_size); }
inline reg_num send_dest(pcounter pc) {
   return pcounter_reg(pc + instr_size + reg_val_size);
}

/* FLOAT a TO b */

inline reg_num float_op(pcounter pc) { return pcounter_reg(pc + instr_size); }
inline reg_num float_dest(pcounter pc) {
   return pcounter_reg(pc + instr_size + reg_val_size);
}

/* ITERATE pred MATCHING */

typedef pcounter iter_match;

inline ptr_val iter_match_object(pcounter pc) {
   return pcounter_ptr(pc + instr_size);
}
inline ptr_val *iter_match_object_pos(pcounter pc) {
   return (ptr_val *)(pc + instr_size);
}
inline predicate_id iter_predicate(pcounter pc) {
   return predicate_get(pc, instr_size + ptr_size);
}
inline reg_num iter_reg(pcounter pc) {
   return pcounter_reg(pc + instr_size + ptr_size + predicate_size);
}
inline bool iter_constant_match(const pcounter pc) {
   return pcounter_bool(pc + instr_size + ptr_size + predicate_size +
                        reg_val_size);
}
inline code_offset_t iter_inner_jump(pcounter pc) {
   return jump_get(
       pc, instr_size + ptr_size + predicate_size + reg_val_size + bool_size);
}
inline code_offset_t iter_outer_jump(pcounter pc) {
   return jump_get(pc, instr_size + ptr_size + predicate_size + reg_val_size +
                           bool_size + jump_size);
}
inline size_t iter_matches_size(const pcounter pc, const size_t base) {
   return (size_t)byte_get(pc, base - count_size);
}

inline utils::byte iter_options(const pcounter pc) {
   return byte_get(pc, ITER_BASE0);
}
inline utils::byte iter_options_argument(const pcounter pc) {
   return byte_get(pc, ITER_BASE0 + sizeof(utils::byte));
}
inline instr_val iter_match_val(iter_match m) {
   return val_get((pcounter)m, sizeof(utils::byte));
}
inline field_num iter_match_field(iter_match m) { return (field_num)*m; }

inline bool iter_options_random(const utils::byte b) { return b & 0x01; }
inline bool iter_options_min(const utils::byte b) { return b & 0x04; }
inline field_num iter_options_min_arg(const utils::byte b) {
   return (field_num)b;
}
inline bool iter_options_to_delete(const utils::byte b) { return b & 0x02; }

/* ALLOC pred to reg */

inline predicate_id alloc_predicate(const pcounter pc) {
   return predicate_get(pc, instr_size);
}
inline reg_num alloc_reg(const pcounter pc) {
   return pcounter_reg(pc + instr_size + predicate_size);
}

inline reg_num alloc_dest(const pcounter pc) {
   return pcounter_reg(pc + instr_size + predicate_size + reg_val_size);
}

/* CALL */

inline external_function_id call_extern_id(pcounter pc) {
   return (external_function_id)byte_get(pc, instr_size);
}
inline reg_num call_dest(pcounter pc) {
   return pcounter_reg(pc + instr_size + extern_id_size);
}
inline utils::byte call_type(pcounter pc) {
   return byte_get(pc, instr_size + extern_id_size + reg_val_size);
}
inline size_t call_gc(pcounter pc) {
   return pcounter_bool(pc + instr_size + extern_id_size + reg_val_size +
                        type_size);
}
inline size_t call_num_args(pcounter pc) {
   return (size_t)byte_get(pc, call_size);
}

/* CALLE (similar to CALL) */

inline external_function_id calle_extern_id(pcounter pc) {
   return (external_function_id)byte_get(pc, 1);
}
inline reg_num calle_dest(pcounter pc) {
   return pcounter_reg(pc + instr_size + extern_id_size);
}
inline utils::byte calle_type(pcounter pc) {
   return byte_get(pc, instr_size + extern_id_size + reg_val_size);
}
inline size_t calle_gc(pcounter pc) {
   return pcounter_bool(pc + instr_size + extern_id_size + reg_val_size +
                        type_size);
}
inline size_t calle_num_args(pcounter pc) {
   return (size_t)byte_get(pc, call_size);
}

/* TEST-NIL a TO b */

inline reg_num test_nil_op(pcounter pc) {
   return pcounter_reg(pc + instr_size);
}
inline reg_num test_nil_dest(pcounter pc) {
   return pcounter_reg(pc + instr_size + reg_val_size);
}

/* CONS (a :: b) TO c */

inline size_t cons_type(pcounter pc) {
   return (size_t)byte_get(pc, instr_size);
}

/* GC a TYPE */

inline reg_num gc_reg(pcounter pc) {
   return pcounter_reg(pc + instr_size + type_size);
}
inline size_t gc_type(pcounter pc) { return (size_t)byte_get(pc, instr_size); }

/* NOT a TO b */

inline reg_num not_op(pcounter pc) { return pcounter_reg(pc + instr_size); }
inline reg_num not_dest(pcounter pc) {
   return pcounter_reg(pc + instr_size + reg_val_size);
}

/* SELECT BY NODE ... */

inline code_size_t select_size(pcounter pc) {
   return pcounter_code_size(pc + instr_size);
}
inline size_t select_hash_size(pcounter pc) {
   return (size_t)pcounter_code_size(pc + instr_size + sizeof(code_size_t));
}
inline pcounter select_hash_start(pcounter pc) { return pc + SELECT_BASE; }
inline code_offset_t select_hash(pcounter hash_start, const node_val val) {
   return pcounter_code_size(hash_start + sizeof(code_size_t) * val);
}
inline pcounter select_hash_code(pcounter hash_start, const size_t hash_size,
                                 const code_offset_t hashed) {
   return hash_start + hash_size * sizeof(code_size_t) + hashed - 1;
}

/* RETURN SELECT */

inline code_size_t return_select_jump(pcounter pc) {
   return pcounter_code_size(pc + 1);
}

/* DELETE predicate */

inline predicate_id delete_predicate(const pcounter pc) {
   return predicate_get(pc, 1);
}
inline size_t delete_num_args(pcounter pc) { return (size_t)byte_get(pc, 2); }
inline instr_val delete_val(pcounter pc) { return val_get(pc, index_size); }
inline field_num delete_index(pcounter pc) { return field_num_get(pc, 0); }

/* RESET LINEAR */

inline code_offset_t reset_linear_jump(const pcounter pc) {
   return jump_get(pc, instr_size);
}

/* RULE ID */

inline size_t rule_get_id(const pcounter pc) {
   return pcounter_uint(pc + instr_size);
}

/* NEW NODE */

inline reg_num new_node_reg(const pcounter pc) {
   return pcounter_reg(pc + instr_size);
}

/* NEW AXIOMS */

inline code_offset_t new_axioms_jump(const pcounter pc) {
   return jump_get(pc, instr_size);
}

/* SEND DELAY a TO b */

inline reg_num send_delay_msg(pcounter pc) {
   return pcounter_reg(pc + instr_size);
}
inline reg_num send_delay_dest(pcounter pc) {
   return pcounter_reg(pc + instr_size + reg_val_size);
}
inline uint_val send_delay_time(pcounter pc) {
   return pcounter_uint(pc + instr_size + 2 * reg_val_size);
}

/* CALLF id */

inline callf_id callf_get_id(const pcounter pc) { return *(pc + instr_size); }

/* MAKE STRUCT size to */

inline size_t make_structr_type(pcounter pc) {
   return (size_t)byte_get(pc, instr_size);
}

/* STRUCT VAL idx FROM a TO b */

inline size_t struct_val_idx(pcounter pc) {
   return (size_t)byte_get(pc, instr_size);
}
inline instr_val struct_val_from(pcounter pc) {
   return val_get(pc, instr_size + sizeof(utils::byte));
}
inline instr_val struct_val_to(pcounter pc) {
   return val_get(pc, instr_size + sizeof(utils::byte) + val_size);
}

/* PUSH N */

inline size_t push_n(pcounter pc) { return (size_t)byte_get(pc, instr_size); }

/* REMOTE UPDATE */

inline reg_num remote_update_dest(const pcounter pc) {
   return predicate_get(pc, instr_size);
}
inline predicate_id remote_update_edit(const pcounter pc) {
   return predicate_get(pc, instr_size + reg_val_size);
}
inline predicate_id remote_update_target(const pcounter pc) {
   return predicate_get(pc, instr_size + reg_val_size + predicate_size);
}
inline size_t remote_update_common(const pcounter pc) {
   return (size_t)byte_get(pc, instr_size + reg_val_size + 2 * predicate_size);
}
inline size_t remote_update_nregs(const pcounter pc) {
   return (size_t)byte_get(
       pc, instr_size + reg_val_size + 2 * predicate_size + count_size);
}

// LITERAL CONS

inline code_offset_t literal_cons_jump(pcounter pc) {
   return jump_get(pc, LITERAL_CONS_BASE - jump_size);
}

inline utils::byte literal_cons_type(pcounter pc) {
   return byte_get(pc, LITERAL_CONS_BASE - jump_size - type_size);
}

inline reg_num literal_cons_dest(const pcounter& pc) {
   return reg_get(pc, instr_size);
}

/* NODE TYPE */

inline int_val node_type_nodes(pcounter pc) {
   return pcounter_uint(pc + instr_size + reg_val_size);
}

inline reg_num node_type_reg(pcounter pc) {
   return reg_get(pc, instr_size);
}

/* advance function */

enum instr_argument_type { ARGUMENT_ANYTHING };

inline pcounter advance(const pcounter pc) {
   switch (fetch(pc)) {
      case SEND_INSTR:
      case THREAD_SEND_INSTR:
         return pc + SEND_BASE;
      case ADDLINEAR_INSTR:
         return pc + ADDLINEAR_BASE;
      case ADDPERS_INSTR:
      case ADDTPERS_INSTR:
         return pc + ADDPERS_BASE;
      case RUNACTION_INSTR:
         return pc + RUNACTION_BASE;
      case ENQUEUE_LINEAR_INSTR:
         return pc + ENQUEUE_LINEAR_BASE;

      case FLOAT_INSTR:
         return pc + FLOAT_BASE;

      case PERS_ITER_INSTR:
      case TPERS_ITER_INSTR:
      case OPERS_ITER_INSTR:
      case LINEAR_ITER_INSTR:
      case TLINEAR_ITER_INSTR:
      case TRLINEAR_ITER_INSTR:
      case RLINEAR_ITER_INSTR:
      case OLINEAR_ITER_INSTR:
      case ORLINEAR_ITER_INSTR:
         return pc + iter_inner_jump(pc);

      case ALLOC_INSTR:
         return pc + ALLOC_BASE;

      case CALL_INSTR:
         return pc + CALL_BASE + call_num_args(pc) * reg_val_size;
      case CALL0_INSTR:
         return pc + CALL0_BASE;
      case CALL1_INSTR:
         return pc + CALL1_BASE;
      case CALL2_INSTR:
         return pc + CALL2_BASE;
      case CALL3_INSTR:
         return pc + CALL3_BASE;

      case CALLE_INSTR:
         return pc + CALLE_BASE + calle_num_args(pc) * reg_val_size;

      case IF_INSTR:
         return pc + IF_BASE;
      case IF_ELSE_INSTR:
         return pc + IF_ELSE_BASE;

      case JUMP_INSTR:
         return pc + JUMP_BASE;

      case TESTNIL_INSTR:
         return pc + TESTNIL_BASE;

      case NOT_INSTR:
         return pc + NOT_BASE;

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
         return pc + SELECT_BASE + select_hash_size(pc) * sizeof(code_size_t);

      case REMOVE_INSTR:
         return pc + REMOVE_BASE;

      case UPDATE_INSTR:
         return pc + UPDATE_BASE;

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

      case PUSHN_INSTR:
         return pc + PUSHN_BASE;

      case POP_INSTR:
         return pc + POP_BASE;

      case PUSH_REGS_INSTR:
         return pc + PUSH_REGS_BASE;

      case POP_REGS_INSTR:
         return pc + POP_REGS_BASE;

      case CALLF_INSTR:
         return pc + CALLF_BASE;

      case MAKE_STRUCTR_INSTR:
         return pc + MAKE_STRUCTR_BASE;
      case MAKE_STRUCTF_INSTR:
         return pc + MAKE_STRUCTF_BASE;

      case STRUCT_VALRR_INSTR:
         return pc + STRUCT_VALRR_BASE;
      case STRUCT_VALFR_INSTR:
         return pc + STRUCT_VALFR_BASE;
      case STRUCT_VALRF_INSTR:
         return pc + STRUCT_VALRF_BASE;
      case STRUCT_VALRFR_INSTR:
         return pc + STRUCT_VALRFR_BASE;
      case STRUCT_VALFF_INSTR:
         return pc + STRUCT_VALFF_BASE;
      case STRUCT_VALFFR_INSTR:
         return pc + STRUCT_VALFFR_BASE;

      case MVINTFIELD_INSTR:
         return pc + MVINTFIELD_BASE;

      case MVINTREG_INSTR:
         return pc + MVINTREG_BASE;

      case MVFIELDFIELD_INSTR:
      case MVFIELDFIELDR_INSTR:
         return pc + MVFIELDFIELD_BASE;

      case MVFIELDREG_INSTR:
         return pc + MVFIELDREG_BASE;

      case MVPTRREG_INSTR:
         return pc + MVPTRREG_BASE;

      case MVNILFIELD_INSTR:
         return pc + MVNILFIELD_BASE;

      case MVNILREG_INSTR:
         return pc + MVNILREG_BASE;

      case MVREGFIELD_INSTR:
      case MVREGFIELDR_INSTR:
         return pc + MVREGFIELD_BASE;

      case MVHOSTFIELD_INSTR:
         return pc + MVHOSTFIELD_BASE;

      case MVREGCONST_INSTR:
         return pc + MVREGCONST_BASE;

      case MVCONSTFIELD_INSTR:
      case MVCONSTFIELDR_INSTR:
         return pc + MVCONSTFIELD_BASE;

      case MVADDRFIELD_INSTR:
         return pc + MVADDRFIELD_BASE;

      case MVFLOATFIELD_INSTR:
         return pc + MVFLOATFIELD_BASE;

      case MVFLOATREG_INSTR:
         return pc + MVFLOATREG_BASE;

      case MVINTCONST_INSTR:
         return pc + MVINTCONST_BASE;

      case MVWORLDFIELD_INSTR:
         return pc + MVWORLDFIELD_BASE;

      case MVSTACKPCOUNTER_INSTR:
         return pc + MVSTACKPCOUNTER_BASE;

      case MVPCOUNTERSTACK_INSTR:
         return pc + MVPCOUNTERSTACK_BASE;

      case MVSTACKREG_INSTR:
         return pc + MVSTACKREG_BASE;

      case MVREGSTACK_INSTR:
         return pc + MVREGSTACK_BASE;

      case MVADDRREG_INSTR:
         return pc + MVADDRREG_BASE;

      case MVHOSTREG_INSTR:
         return pc + MVHOSTREG_BASE;

      case MVFLOATSTACK_INSTR:
         return pc + MVFLOATSTACK_BASE;

      case MVARGREG_INSTR:
         return pc + MVARGREG_BASE;

      case ADDRNOTEQUAL_INSTR:
      case ADDREQUAL_INSTR:
      case INTMINUS_INSTR:
      case INTEQUAL_INSTR:
      case INTNOTEQUAL_INSTR:
      case INTPLUS_INSTR:
      case INTLESSER_INSTR:
      case INTGREATEREQUAL_INSTR:
      case INTLESSEREQUAL_INSTR:
      case INTGREATER_INSTR:
      case INTMUL_INSTR:
      case INTDIV_INSTR:
      case INTMOD_INSTR:
      case FLOATPLUS_INSTR:
      case FLOATMINUS_INSTR:
      case FLOATMUL_INSTR:
      case FLOATDIV_INSTR:
      case FLOATEQUAL_INSTR:
      case FLOATNOTEQUAL_INSTR:
      case FLOATLESSER_INSTR:
      case FLOATLESSEREQUAL_INSTR:
      case FLOATGREATER_INSTR:
      case FLOATGREATEREQUAL_INSTR:
      case BOOLOR_INSTR:
      case BOOLEQUAL_INSTR:
      case BOOLNOTEQUAL_INSTR:
         return pc + operation_size;

      case MVREGREG_INSTR:
         return pc + MVREGREG_BASE;

      case HEADRR_INSTR:
         return pc + HEADRR_BASE;

      case HEADFR_INSTR:
         return pc + HEADFR_BASE;

      case HEADFF_INSTR:
      case HEADFFR_INSTR:
         return pc + HEADFF_BASE;

      case HEADRF_INSTR:
      case HEADRFR_INSTR:
         return pc + HEADRF_BASE;

      case TAILRR_INSTR:
         return pc + TAILRR_BASE;

      case TAILFR_INSTR:
         return pc + TAILFR_BASE;

      case TAILFF_INSTR:
         return pc + TAILFF_BASE;

      case TAILRF_INSTR:
         return pc + TAILRF_BASE;

      case MVWORLDREG_INSTR:
         return pc + MVWORLDREG_BASE;

      case MVCONSTREG_INSTR:
         return pc + MVCONSTREG_BASE;

      case MVINTSTACK_INSTR:
         return pc + MVINTSTACK_BASE;

      case CONSRRR_INSTR:
         return pc + CONSRRR_BASE;

      case CONSRFF_INSTR:
         return pc + CONSRFF_BASE;

      case CONSFRF_INSTR:
         return pc + CONSFRF_BASE;

      case CONSFFR_INSTR:
         return pc + CONSFFR_BASE;

      case CONSRRF_INSTR:
         return pc + CONSRRF_BASE;

      case CONSRFR_INSTR:
         return pc + CONSRFR_BASE;

      case CONSFRR_INSTR:
         return pc + CONSFRR_BASE;

      case CONSFFF_INSTR:
         return pc + CONSFFF_BASE;

      case SET_PRIORITY_INSTR:
         return pc + SET_PRIORITY_BASE;

      case SET_PRIORITYH_INSTR:
         return pc + SET_PRIORITYH_BASE;

      case ADD_PRIORITY_INSTR:
         return pc + ADD_PRIORITY_BASE;

      case ADD_PRIORITYH_INSTR:
         return pc + ADD_PRIORITYH_BASE;

      case STOP_PROG_INSTR:
         return pc + STOP_PROG_BASE;

      case CPU_ID_INSTR:
         return pc + CPU_ID_BASE;

      case NODE_PRIORITY_INSTR:
         return pc + NODE_PRIORITY_BASE;

      case MVSTACKFIELD_INSTR:
         return pc + MVSTACKFIELD_BASE;

      case SET_DEFPRIOH_INSTR:
         return pc + SET_DEFPRIOH_BASE;

      case SET_DEFPRIO_INSTR:
         return pc + SET_DEFPRIO_BASE;

      case SET_STATICH_INSTR:
         return pc + SET_STATICH_BASE;

      case SET_STATIC_INSTR:
         return pc + SET_STATIC_BASE;

      case SET_MOVINGH_INSTR:
         return pc + SET_MOVINGH_BASE;

      case SET_MOVING_INSTR:
         return pc + SET_MOVING_BASE;

      case SET_AFFINITYH_INSTR:
         return pc + SET_AFFINITYH_BASE;

      case SET_AFFINITY_INSTR:
         return pc + SET_AFFINITY_BASE;

      case CPU_STATIC_INSTR:
         return pc + CPU_STATIC_BASE;

      case MVCPUSREG_INSTR:
         return pc + MVCPUSREG_BASE;

      case SET_CPUH_INSTR:
         return pc + SET_CPUH_BASE;

      case IS_STATIC_INSTR:
         return pc + IS_STATIC_BASE;

      case IS_MOVING_INSTR:
         return pc + IS_MOVING_BASE;

      case REM_PRIORITY_INSTR:
         return pc + REM_PRIORITY_BASE;

      case REM_PRIORITYH_INSTR:
         return pc + REM_PRIORITYH_BASE;

      case FACTS_PROVED_INSTR:
         return pc + FACTS_PROVED_BASE;

      case FACTS_CONSUMED_INSTR:
         return pc + FACTS_CONSUMED_BASE;

      case SCHEDULE_NEXT_INSTR:
         return pc + SCHEDULE_NEXT_BASE;

      case FABS_INSTR:
         return pc + FABS_BASE;

      case REMOTE_UPDATE_INSTR:
         return pc + REMOTE_UPDATE_BASE +
                remote_update_nregs(pc) * reg_val_size;

      case NODE_TYPE_INSTR:
         return pc + NODE_TYPE_BASE +
                node_type_nodes(pc) * val_size;

      case MVTYPEREG_INSTR:
         return pc + MVTYPEREG_BASE;

      case MVTHREADIDREG_INSTR:
         return pc + MVTHREADIDREG_BASE;

      case MVTHREADIDFIELD_INSTR:
         return pc + MVTHREADIDFIELD_BASE;

      case MARK_RULE_INSTR:
         return pc + MARK_RULE_BASE;

      case LITERAL_CONS_INSTR:
         return pc + literal_cons_jump(pc);

      default:
         throw malformed_instr_error("unknown instruction code (advance)");
   }
}
}

/* byte code print functions */
std::string instr_name(const instr::instr_type);
pcounter instr_print(pcounter, const bool, const int, const program *,
                     std::ostream &);
pcounter instr_print_simple(pcounter, const int, const program *,
                            std::ostream &);
byte_code instrs_print(const byte_code, const code_size_t, const int,
                       const program *, std::ostream &);
}

#endif
