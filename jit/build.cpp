
#ifdef USE_JIT

#include <jit/jit-dump.h>
#include <iostream>

#include "jit/build.hpp"

using namespace vm;
using namespace std;
using namespace instr;


namespace jit
{

typedef struct {
   jit_value_t val;
   field_type typ;
} reg_val;

typedef std::map<reg_num, reg_val> reg_map;
typedef std::map<reg_num, const predicate*> pred_map;
static bool created_based_types = false;
static jit_type_t reg_type;
static jit_type_t list_type;
static jit_type_t intrusive_list_type;
static jit_type_t tuple_type;
static jit_function_t function;
static void inner_compile_bytecode(byte_code, const size_t, vm::state&, jit_function_t *, reg_map&, pred_map&, jit_label_t *);

static jit_type_t
field_type_to_jit_type(const field_type typ)
{
   switch(typ) {
      case FIELD_BOOL: return jit_type_uint;
      case FIELD_INT: return jit_type_int;
      case FIELD_FLOAT: return jit_type_float64;
      case FIELD_LIST:
      case FIELD_STRUCT:
      case FIELD_STRING:
      case FIELD_NODE: return jit_type_void_ptr;
      default: assert(false);
   }
   return jit_type_int;
}

#if 0
static void
print_tuple(vm::tuple *tpl)
{
   cout << *tpl << endl;
}
#endif

#if 0
static void
print_ptr(void *ptr)
{
   cout << ptr << endl;
}
#endif

pcounter
compile_instruction(const pcounter pc, state& state, jit_function_t *f, reg_map& regs, pred_map& preds, jit_label_t *done)
{
   switch(fetch(pc)) {
      case RULE_INSTR:
         break;
      case NEXT_INSTR:
         break;
      case RETURN_INSTR:
         jit_insn_return(*f, NULL);
         break;
      case LINEAR_ITER_INSTR: {
         // XXX assume no match object
         const predicate *pred(state.all->PROGRAM->get_predicate(iter_predicate(pc)));
         const reg_num reg(iter_reg(pc));
         // find linear list through state
         jit_value_t lists = jit_value_get_param(*f, 1);
         jit_value_t list_data = jit_insn_load_relative(*f, lists, jit_type_get_offset(list_type, 0), jit_type_void_ptr);
         jit_value_t pred_list = jit_insn_add_relative(*f, list_data, sizeof(db::lists::tuple_list) * pred->get_id());
         jit_value_t head_list = jit_insn_load_relative(*f, pred_list, jit_type_get_offset(intrusive_list_type, 0), jit_type_void_ptr);

         jit_label_t test_label = jit_label_undefined;
         jit_label_t end_label = jit_label_undefined;
         jit_label_t success_label = jit_label_undefined;
         jit_insn_label(*f, &test_label);
         jit_value_t null = jit_value_create_long_constant(*f, jit_type_void_ptr, 0);
         jit_value_t is_null = jit_insn_eq(*f, null, head_list);
         jit_insn_branch_if(*f, is_null, &end_label);

         // iteration
#if 0
         jit_type_t params[1];
         params[0] = jit_type_void_ptr; /* node */
         jit_type_t signature = jit_type_create_signature(jit_abi_cdecl, jit_type_void, params, sizeof(params)/sizeof(jit_type_t), 0);
         jit_value_t args[] = {head_list};
         jit_insn_call_native(*f, "print_tuple", (void*)print_tuple, signature, args, 1, 0);
         jit_type_free(signature);
#endif
         reg_map copy(regs);
         pred_map copy_preds(preds);
         regs[reg] = {head_list, FIELD_NODE}; // this is actually a tuple
         preds[reg] = pred;
         inner_compile_bytecode(pc + iter_inner_jump(pc), iter_outer_jump(pc) - iter_inner_jump(pc), state, f, regs, preds, &success_label);
         regs = copy;
         preds = copy_preds;

         jit_label_t next_label = jit_label_undefined;
         jit_insn_label(*f, &next_label);
         jit_value_t new_head_list = jit_insn_load_relative(*f, head_list, jit_type_get_offset(tuple_type, 0), jit_type_void_ptr);
         jit_insn_store(*f, head_list, new_head_list);
         jit_insn_branch(*f, &test_label);

         jit_insn_label(*f, &success_label);
         if(done) {
            jit_insn_branch(*f, done);
         }
         jit_insn_branch(*f, &next_label);

         jit_insn_label(*f, &end_label);

         return pc + iter_outer_jump(pc);
      }
      break;
      case MVINTREG_INSTR: {
         const int_val i(pcounter_int(pc + instr_size));
         const reg_num reg(pcounter_reg(pc + instr_size + int_size));

         reg_map::iterator it2(regs.find(reg));
         jit_value_t toval;
         if(it2 == regs.end()) {
            toval = jit_value_create(*f, jit_type_int);
            regs[reg] = {toval, FIELD_INT};
         } else {
            if(it2->second.typ != FIELD_INT) {
               toval = jit_value_create(*f, jit_type_int);
               regs[reg] = {toval, FIELD_INT};
            } else {
               toval = it2->second.val;
            }
         }

         jit_value_t icons(jit_value_create_nint_constant(*f, jit_type_int, i));
         jit_insn_store(*f, toval, icons);
      }
      break;
      case MVFIELDREG_INSTR: {
         const field_num from(val_field_num(pc + instr_size));
         const reg_num reg(val_field_reg(pc + instr_size));
         const reg_num to(pcounter_reg(pc + instr_size + field_size));

         reg_map::iterator it(regs.find(reg));
         assert(it != regs.end());
         jit_value_t reg_val(it->second.val);
         pred_map::iterator it_pred(preds.find(reg));
         assert(it_pred != preds.end());
         const predicate *pred(it_pred->second);

         jit_value_t args(jit_insn_add_relative(*f, reg_val, jit_type_get_size(tuple_type)));
         jit_value_t idx(jit_value_create_nint_constant(*f, jit_type_int, (int)from));
         jit_value_t arg(jit_insn_load_elem(*f, args, idx, reg_type));
         reg_map::iterator it2(regs.find(to));

         jit_value_t toval;
         if(it2 == regs.end()) {
            toval = jit_value_create(*f, field_type_to_jit_type(pred->get_field_type(from)->get_type()));
            regs[to] = {toval, pred->get_field_type(from)->get_type()};
         } else {
            if(it2->second.typ != pred->get_field_type(from)->get_type()) {
               toval = jit_value_create(*f, field_type_to_jit_type(pred->get_field_type(from)->get_type()));
               regs[to] = {toval, pred->get_field_type(from)->get_type()};
            } else
               toval = it2->second.val;
         }
         jit_insn_store(*f, toval, arg);
         break;
      }
      case RETURN_DERIVED_INSTR: {
         assert(done);
         jit_insn_branch(*f, done);
         break;
      }
      case UPDATE_INSTR: {
         const reg_num reg(pcounter_reg(pc + instr_size));
         reg_map::iterator it(regs.find(reg));
         assert(it != regs.end());
         jit_value_t store = jit_value_get_param(*f, 2);
         jit_value_t boolpreds = jit_insn_load_relative(*f, store, jit_type_get_offset(list_type, 0), jit_type_void_ptr);
         assert(sizeof(bool) == jit_type_get_size(jit_type_ubyte));
         pred_map::iterator itp(preds.find(reg));
         assert(itp != preds.end());
         const predicate *pred(itp->second);
         jit_value_t idx(jit_value_create_nint_constant(*f, jit_type_int, pred->get_id()));
         jit_value_t true_val(jit_value_create_nint_constant(*f, jit_type_ubyte, 1));
         jit_insn_store_elem(*f, boolpreds, idx, true_val);
//         jit_value_t tuple_val(it->second.val);
         break;
      }
      case INTMINUS_INSTR: {
         const reg_num r1(pcounter_reg(pc + instr_size));
         const reg_num r2(pcounter_reg(pc + instr_size + reg_val_size));
         const reg_num dst(pcounter_reg(pc + instr_size + 2 * reg_val_size));

         reg_map::iterator it1(regs.find(r1));
         reg_map::iterator it2(regs.find(r2));
         assert(it1 != regs.end());
         assert(it2 != regs.end());
         assert(it1->second.typ == FIELD_INT);
         assert(it2->second.typ == FIELD_INT);

         jit_value_t dest(jit_insn_sub(*f, it1->second.val, it2->second.val));

         regs[dst] = {dest, FIELD_BOOL};
         break;
      }
      case MVREGFIELD_INSTR: {
         const reg_num reg(pcounter_reg(pc + instr_size));
         const field_num field(val_field_num(pc + instr_size + reg_val_size));
         const reg_num tuple_reg(val_field_reg(pc + instr_size + reg_val_size));
         reg_map::iterator it(regs.find(tuple_reg));
         assert(it != regs.end());
         jit_value_t tuple_val(it->second.val);
#if 0
         pred_map::iterator it_pred(preds.find(reg));
         assert(it_pred != preds.end());
         const predicate *pred(it_pred->second);
#endif

         reg_map::iterator it2(regs.find(reg));
         assert(it2 != regs.end());
         jit_value_t reg_val(it2->second.val);

         jit_value_t args(jit_insn_add_relative(*f, tuple_val, jit_type_get_size(tuple_type)));
         jit_value_t arg(jit_insn_add_relative(*f, args, field * jit_type_get_size(reg_type)));
         jit_insn_store_relative(*f, arg, 0, reg_val);
         break;
      }
      case INTGREATER_INSTR: {
         const reg_num r1(pcounter_reg(pc + instr_size));
         const reg_num r2(pcounter_reg(pc + instr_size + reg_val_size));
         const reg_num dst(pcounter_reg(pc + instr_size + 2 * reg_val_size));

         reg_map::iterator it1(regs.find(r1));
         reg_map::iterator it2(regs.find(r2));
         assert(it1 != regs.end());
         assert(it2 != regs.end());
         assert(it1->second.typ == FIELD_INT);
         assert(it2->second.typ == FIELD_INT);

         jit_value_t dest(jit_insn_gt(*f, it1->second.val, it2->second.val));

         regs[dst] = {dest, FIELD_BOOL};
         break;
      }
      case IF_INSTR: {
         const reg_num reg(if_reg(pc));
         reg_map::iterator it(regs.find(reg));
         assert(it != regs.end());
         assert(it->second.typ == FIELD_BOOL);
         jit_label_t else_label = jit_label_undefined;
         jit_insn_branch_if_not(*f, it->second.val, &else_label);

         inner_compile_bytecode(advance(pc), if_jump(pc) - (advance(pc) - pc), state, f, regs, preds, done);

         jit_insn_label(*f, &else_label);
         return pc + if_jump(pc);
      }

      default: printf("FAIL ");
               break;
   }

   return advance(pc);
}

static void
inner_compile_bytecode(byte_code code, const size_t len, vm::state& state, jit_function_t *f, reg_map& regs, pred_map& preds, jit_label_t *done)
{
   pcounter pc = code;
   pcounter until = code + len;

	for (; pc < until; )
      pc = compile_instruction(pc, state, f, regs, preds, done);
}

void
compile_bytecode(byte_code code, const size_t len, vm::state& state)
{
   reg_map regs;
   pred_map preds;

   jit_init();
   jit_context_t ctx = jit_context_create();

   if(!created_based_types) {
      created_based_types = true;

      // create union type for registers
      jit_type_t available_reg_types[] = {jit_type_int, jit_type_float64, jit_type_void_ptr, 
         jit_type_uint};
      reg_type = jit_type_create_union(available_reg_types, sizeof(available_reg_types)/sizeof(jit_type_t), 0);
      assert(jit_type_get_size(jit_type_int) == sizeof(int_val));
      assert(jit_type_get_size(jit_type_uint) == sizeof(uint_val));
      assert(jit_type_get_size(jit_type_float64) == sizeof(float_val));
      assert(jit_type_get_size(jit_type_void_ptr) == sizeof(ptr_val));

      // create type for db::linear_store
      jit_type_t list_type_fields[] = {jit_type_void_ptr, jit_type_ulong};
      assert(jit_type_get_size(jit_type_ulong) == sizeof(size_t));
      list_type = jit_type_create_struct(list_type_fields, sizeof(list_type_fields)/sizeof(jit_type_t), 0);

      // create type for db::intrusive_list
      jit_type_t intrusive_list_type_fields[] = {jit_type_void_ptr, jit_type_void_ptr};
      intrusive_list_type = jit_type_create_struct(intrusive_list_type_fields, sizeof(intrusive_list_type_fields)/sizeof(jit_type_t), 0);

      // create tuple type
      jit_type_t tuple_type_fields[] = {jit_type_void_ptr, jit_type_void_ptr, jit_type_ubyte, jit_type_void_ptr};
      tuple_type = jit_type_create_struct(tuple_type_fields, sizeof(tuple_type_fields)/sizeof(jit_type_t), 0);
      assert(jit_type_get_size(jit_type_ubyte) == sizeof(utils::byte));
      assert(jit_type_get_size(tuple_type) == sizeof(vm::tuple));

   jit_context_build_start(ctx);

   jit_type_t signature;
   jit_type_t params[3];
   params[0] = jit_type_void_ptr; /* node */
   params[1] = jit_type_void_ptr; /* lists */
   params[2] = jit_type_void_ptr; /* lists */
   signature = jit_type_create_signature(jit_abi_cdecl, jit_type_void, params, sizeof(params)/sizeof(jit_type_t), 0);
   function = jit_function_create(ctx, signature);

   inner_compile_bytecode(code, len, state, &function, regs, preds, NULL);

#if 0
   jit_dump_function(stdout, function, "fun");
#endif
   jit_function_compile(function);
   jit_context_build_end(ctx);
#if 0
   jit_dump_function(stdout, function, "fun");
#endif
   }

   void *node = state.node;
   void *ls = state.lists;
   void *store = state.store;
   void *args[3] = {&node, &ls, &store};
   jit_int result = 0;
#if 0
   for(size_t i(0); i < state.all->PROGRAM->num_predicates(); ++i) {
      cout << i << " " << state.store->predicates[i] << endl;
   }
#endif
   jit_function_apply(function, args, &result);
#if 0
   for(size_t i(0); i < state.all->PROGRAM->num_predicates(); ++i) {
      cout << i << " " << state.store->predicates[i] << endl;
   }
#endif
}

}
#endif // USE_JIT
