
#include "vm/predicate.hpp"
#include "vm/instr.hpp"
#include "vm/program.hpp"
#include "utils/utils.hpp"
#include "db/node.hpp"

using namespace std;
using namespace vm;
using namespace utils;

namespace vm {

bool USING_MEM_ADDRESSES = true;

namespace instr {

static inline string reg_string(const reg_num num) {
   return string("reg ") + to_string((int)num);
}

static inline string argument_string(const argument_id id) {
   return string("@arg ") + to_string((int)id);
}

static inline string operation_string(pcounter& pc, const string& op) {
   return string(" ") + reg_string(pcounter_reg(pc + instr_size)) + " " + op +
          " " + reg_string(pcounter_reg(pc + instr_size + reg_val_size)) +
          " TO " + reg_string(pcounter_reg(pc + instr_size + 2 * reg_val_size));
}

static inline string call_string(const string& extra, pcounter& pc,
                                 const size_t args) {
   external_function* f(lookup_external_function(call_extern_id(pc)));
   return extra + " " + f->get_name() + "/" + to_string(args) + " TO " +
          reg_string(call_dest(pc)) + " " + (call_gc(pc) ? "GC " : "") + "= (";
}

static string val_string(const instr_val, pcounter*, const program*);

static string list_string(pcounter* pm, const program* prog) {
   const instr_val head_val(val_get(*pm, 0));

   pcounter_move_byte(pm);
   const string head_string(val_string(head_val, pm, prog));
   const instr_val tail_val(val_get(*pm, 0));
   pcounter_move_byte(pm);
   const string tail_string(val_string(tail_val, pm, prog));

   return string("[") + head_string + " | " + tail_string + string("]");
}

static inline string int_string(const int_val i) {
   return string("INT ") + to_string(i);
}

static inline string bool_string(const bool_val b) {
   return b ? string("TRUE") : string("FALSE");
}

static inline string ptr_string(const ptr_val p) {
   return string("PTR ") + to_string(p);
}

static inline string const_id_string(const const_id id) {
   return string("CONST ") + to_string(id);
}

static inline string node_string(const node_val n) {
   return string("@") + to_string(n);
}

static inline string stack_string(const offset_num s) {
   return string("STACK ") + to_string((int)s);
}

static inline string float_string(const float_val f) {
   return string("FLOAT ") + to_string(f);
}

static inline string field_string(pcounter pc) {
   return to_string((int)val_field_reg(pc)) + string(".") +
          to_string((int)val_field_num(pc));
}

static inline string field_string(pcounter* pm) {
   const string ret(field_string(*pm));
   pcounter_move_field(pm);
   return ret;
}

static string val_string(const instr_val v, pcounter* pm, const program* prog) {
   if (val_is_reg(v))
      return reg_string(val_reg(v));
   else if (val_is_host(v))
      return string("host");
   else if (val_is_nil(v))
      return string("nil");
   else if (val_is_non_nil(v))
      return string("non nil");
   else if (val_is_any(v))
      return string("...");
   else if (val_is_list(v))
      return list_string(pm, prog);
   else if (val_is_field(v))
      return field_string(pm);
   else if (val_is_int(v)) {
      const string ret(int_string(pcounter_int(*pm)));
      pcounter_move_int(pm);
      return ret;
   } else if (val_is_bool(v)) {
      const bool_val v = pcounter_bool(*pm);
      pcounter_move_bool(pm);
      return string("BOOL ") + (v ? "true" : "false");
   } else if (val_is_float(v)) {
      const string ret(float_string(pcounter_float(*pm)));
      pcounter_move_float(pm);
      return ret;
   } else if (val_is_node(v)) {
      const string ret(node_string(pcounter_node(*pm)));
      pcounter_move_node(pm);
      return ret;
   } else if (val_is_string(v)) {
      const uint_val id(pcounter_uint(*pm));
      runtime::rstring::ptr p(prog->get_default_string(id));

      assert(p != nullptr);
      const string ret(string("\"") + p->get_content() + "\"");
      pcounter_move_uint(pm);
      return ret;
   } else if (val_is_arg(v)) {
      const string ret(argument_string(pcounter_argument_id(*pm)));

      pcounter_move_argument_id(pm);

      return ret;
   } else if (val_is_stack(v)) {
      const offset_num offset(pcounter_stack(*pm));

      pcounter_move_offset_num(pm);

      return stack_string(offset);
   } else if (val_is_pcounter(v)) {
      return string("PCOUNTER");
   } else if (val_is_const(v)) {
      const string ret(const_id_string(pcounter_const_id(*pm)));

      pcounter_move_const_id(pm);

      return ret;
   } else if (val_is_ptr(v)) {
      const string ret(ptr_string(pcounter_ptr(*pm)));

      pcounter_move_ptr(pm);

      return ret;
   } else
      throw type_error("Unrecognized val type " + to_string(v) +
                       " (val_string)");

   return string("");
}
}

using namespace vm::instr;

static inline void instrs_print_until_return_select(byte_code code,
                                                    const int tabcount,
                                                    const program* prog,
                                                    ostream& cout) {
   pcounter pc = code;
   pcounter new_pc;

   while (true) {
      new_pc = instr_print(pc, true, tabcount, prog, cout);
      if (fetch(pc) == RETURN_SELECT_INSTR) return;
      pc = new_pc;
   }
}

static inline pcounter instrs_print_until_end_linear(byte_code code,
                                                     const int tabcount,
                                                     const program* prog,
                                                     ostream& cout) {
   pcounter pc = code;
   pcounter new_pc;

   while (true) {
      new_pc = instr_print(pc, true, tabcount, prog, cout);
      if (fetch(pc) == END_LINEAR_INSTR) return pc;
      pc = new_pc;
   }
}

static inline pcounter instrs_print_until(byte_code code, byte_code until,
                                          const int tabcount,
                                          const program* prog, ostream& cout) {
   pcounter pc = code;

   for (; pc < until;) {
      pc = instr_print(pc, true, tabcount, prog, cout);
   }

   return until;
}

static inline void print_tab(const int tabcount) {
   for (int i = 0; i < tabcount; ++i) cout << "  ";
}

static inline void print_axiom_data(pcounter& p, type* t,
                                    bool in_list = false) {
   switch (t->get_type()) {
      case FIELD_INT:
         cout << pcounter_int(p);
         pcounter_move_int(&p);
         break;
      case FIELD_FLOAT:
         cout << pcounter_float(p);
         pcounter_move_float(&p);
         break;
      case FIELD_NODE:
#ifdef USE_REAL_NODES
         if (USING_MEM_ADDRESSES)
            cout << "@" << ((db::node*)pcounter_node(p))->get_id();
         else
#endif
            cout << "@" << pcounter_node(p);
         pcounter_move_node(&p);
         break;
      case FIELD_LIST: {
         if (!in_list) {
            cout << "[";
         }
         if (*p++ == 0) {
            cout << "]";
            break;
         }
         list_type* lt((list_type*)t);
         print_axiom_data(p, lt->get_subtype());
         if (*p == 1) cout << ", ";
         print_axiom_data(p, lt, true);
      } break;
      case FIELD_STRUCT: {
         struct_type *st((struct_type *)t);
         cout << ":(";
         for (size_t i(0); i < st->get_size(); ++i) {
            if(i)
               cout << ", ";
            print_axiom_data(p, st->get_type(i));
         }
         cout << ")";
      } break;
      default:
         assert(false);
   }
}

static inline void print_iter_matches(const pcounter pc, const size_t base,
                                      const size_t tabcount,
                                      const program* prog) {
   pcounter m = pc + base;
   const size_t iter_matches(iter_matches_size(pc, base));

   for (size_t i(0); i < iter_matches; ++i) {
      print_tab(tabcount + 1);
      const instr_val mval(iter_match_val(m));
      const field_num field(iter_match_field(m));
      m += iter_match_size;
      cout << "  (match)." << field << "=" << val_string(mval, &m, prog)
           << endl;
   }
}

string instr_name(const instr::instr_type code) {
   switch (code) {
      case RETURN_INSTR:
         return string("RETURN");
      case RETURN_LINEAR_INSTR:
         return string("RETURN LINEAR");
      case RETURN_DERIVED_INSTR:
         return string("RETURN DERIVED");
      case END_LINEAR_INSTR:
         return string("END LINEAR");
      case RESET_LINEAR_INSTR:
         return string("RESET LINEAR");
      case NEXT_INSTR:
         return string("NEXT");
      case IF_ELSE_INSTR:
      case IF_INSTR:
         return string("IF");
      case TESTNIL_INSTR:
         return string("TESTNIL");
      case ALLOC_INSTR:
         return string("ALLOC");
      case FLOAT_INSTR:
         return string("FLOAT");
      case THREAD_SEND_INSTR:
         return string("THREAD SEND");
      case SEND_INSTR:
         return string("SEND");
      case ADDLINEAR_INSTR:
         return string("ADDLINEAR");
      case PUSH_INSTR:
         return string("PUSH");
      case PUSHN_INSTR:
         return string("PUSHN");
      case POP_INSTR:
         return string("POP");
      case PUSH_REGS_INSTR:
         return string("PUSH REGS");
      case POP_REGS_INSTR:
         return string("POP REGS");
      case ADDPERS_INSTR:
         return string("ADD PERSISTENT");
      case ADDTPERS_INSTR:
         return string("ADD THREAD PERSISTENT");
      case RUNACTION_INSTR:
         return string("RUN ACTION");
      case ENQUEUE_LINEAR_INSTR:
         return string("ENQUEUE LINEAR");
      case PERS_ITER_INSTR:
         return string("PERSISTENT ITERATE");
      case TPERS_ITER_INSTR:
         return string("THREAD PERSISTENT ITERATE");
      case LINEAR_ITER_INSTR:
         return string("LINEAR ITERATE");
      case TLINEAR_ITER_INSTR:
         return string("THREAD LINEAR ITERATE");
      case TRLINEAR_ITER_INSTR:
         return string("THREAD LINEAR(R) ITERATE");
      case RLINEAR_ITER_INSTR:
         return string("LINEAR(R) ITERATE");
      case OPERS_ITER_INSTR:
         return string("ORDER PERSISTENT ITERATE");
      case OLINEAR_ITER_INSTR:
         return string("ORDER LINEAR ITERATE");
      case ORLINEAR_ITER_INSTR:
         return string("ORDER LINEAR(R) ITERATE");
      case SCHEDULE_NEXT_INSTR:
         return string("SCHEDULE NEXT");
      case CALL_INSTR:
      case CALL0_INSTR:
      case CALL1_INSTR:
      case CALL2_INSTR:
      case CALL3_INSTR:
         return string("CALL");
      case CALLE_INSTR:
         return string("CALLE");
      case REMOVE_INSTR:
         return string("REMOVE");
      case UPDATE_INSTR:
         return string("UPDATE");
      case NOT_INSTR:
         return string("NOT");
      case LITERAL_CONS_INSTR:
         return string("LITERAL CONS");
      case RETURN_SELECT_INSTR:
         return string("RETURN SELECT");
      case SELECT_INSTR:
         return string("SELECT BY NODE");
      case RULE_INSTR:
         return string("RULE");
      case MARK_RULE_INSTR:
         return string("MARK RULE");
      case RULE_DONE_INSTR:
         return string("RULE DONE");
      case NEW_NODE_INSTR:
         return string("NEW NODE");
      case NEW_AXIOMS_INSTR:
         return string("NEW AXIOMS");
      case SEND_DELAY_INSTR:
         return string("SEND DELAY");
      case CALLF_INSTR:
         return string("CALLF");
      case MAKE_STRUCTR_INSTR:
         return string("MAKE STRUCT");
      case MAKE_STRUCTF_INSTR:
         return string("MAKE STRUCTF");
      case STRUCT_VALRR_INSTR:
         return string("STRUCT VALRR");
      case STRUCT_VALFR_INSTR:
         return string("STRUCT VALFR");
      case STRUCT_VALRF_INSTR:
         return string("STRUCT VALRF");
      case STRUCT_VALRFR_INSTR:
         return string("STRUCT VALRF");
      case STRUCT_VALFF_INSTR:
         return string("STRUCT VALFF");
      case STRUCT_VALFFR_INSTR:
         return string("STRUCT VALFFR");
      case MVINTFIELD_INSTR:
         return string("MVINTFIELD");
      case MVTYPEREG_INSTR:
         return string("MVTYPEREG");
      case MVTHREADIDREG_INSTR:
         return string("MVTHREADIDREG");
      case MVTHREADIDFIELD_INSTR:
         return string("MVTHREADIDFIELD");
      case MVFIELDFIELD_INSTR:
         return string("MVFIELDFIELD");
      case MVFIELDFIELDR_INSTR:
         return string("MVFIELDFIELDR");
      case MVINTREG_INSTR:
         return string("MVINTREG");
      case MVFIELDREG_INSTR:
         return string("MVFIELDREG");
      case MVPTRREG_INSTR:
         return string("MVPTRREG");
      case MVNILFIELD_INSTR:
         return string("MVNILFIELD");
      case MVNILREG_INSTR:
         return string("MVNILREG");
      case MVREGFIELD_INSTR:
         return string("MVREGFIELD");
      case MVREGFIELDR_INSTR:
         return string("MVREGFIELD");
      case MVHOSTFIELD_INSTR:
         return string("MVHOSTFIELD");
      case MVREGCONST_INSTR:
         return string("MVREGCONST");
      case MVCONSTFIELD_INSTR:
         return string("MVCONSTFIELD");
      case MVCONSTFIELDR_INSTR:
         return string("MVCONSTFIELDR");
      case MVADDRFIELD_INSTR:
         return string("MVADDRFIELD");
      case MVFLOATFIELD_INSTR:
         return string("MVFLOATFIELD");
      case MVFLOATREG_INSTR:
         return string("MVFLOATREG");
      case MVINTCONST_INSTR:
         return string("MVINTCONST");
      case MVWORLDFIELD_INSTR:
         return string("MVWORLDFIELD");
      case MVSTACKPCOUNTER_INSTR:
         return string("MVSTACKCOUNTER");
      case MVPCOUNTERSTACK_INSTR:
         return string("MVPCOUNTERSTACK");
      case MVSTACKREG_INSTR:
         return string("MVSTACKREG");
      case MVREGSTACK_INSTR:
         return string("MVREGSTACK");
      case MVADDRREG_INSTR:
         return string("MVADDRREG");
      case MVHOSTREG_INSTR:
         return string("MVHOSTREG");
      case MVINTSTACK_INSTR:
         return string("MVINTSTACK");
      case MVARGREG_INSTR:
         return string("MVARGREG");
      case ADDRNOTEQUAL_INSTR:
      case ADDREQUAL_INSTR:
      case INTMINUS_INSTR:
      case INTEQUAL_INSTR:
      case INTNOTEQUAL_INSTR:
      case INTPLUS_INSTR:
      case INTLESSER_INSTR:
      case INTGREATEREQUAL_INSTR:
      case BOOLOR_INSTR:
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
      case BOOLEQUAL_INSTR:
      case BOOLNOTEQUAL_INSTR:
         return string("OP");
      case REM_PRIORITYH_INSTR:
         return string("REMOVE PRIORITY FROM NODE");
      case MVREGREG_INSTR:
         return string("MVREGREG");
      case HEADRR_INSTR:
         return string("HEADRR");
      case HEADFR_INSTR:
         return string("HEADFR");
      case HEADFF_INSTR:
         return string("HEADFF");
      case HEADRF_INSTR:
         return string("HEADRF");
      case HEADFFR_INSTR:
         return string("HEADFF");
      case HEADRFR_INSTR:
         return string("HEADRF");
      case TAILRR_INSTR:
         return string("TAILRR");
      case TAILFR_INSTR:
         return string("TAILFR");
      case TAILFF_INSTR:
         return string("TAILFF");
      case TAILRF_INSTR:
         return string("TAILRF");
      case MVWORLDREG_INSTR:
         return string("MVWORLDREG");
      case MVCONSTREG_INSTR:
         return string("MVCONSTREG");
      case CONSRRR_INSTR:
         return string("CONSRRR");
      case CONSRFF_INSTR:
         return string("CONSRFF");
      case CONSFRF_INSTR:
         return string("CONSFRF");
      case CONSFFR_INSTR:
         return string("CONSFFR");
      case CONSRRF_INSTR:
         return string("CONSRRF");
      case CONSRFR_INSTR:
         return string("CONSRFR");
      case CONSFRR_INSTR:
         return string("CONSFRR");
      case CONSFFF_INSTR:
         return string("CONSFFF");
      case MVFLOATSTACK_INSTR:
         return string("MVFLOATSTACK");
      case SET_PRIORITY_INSTR:
         return string("SET PRIORITY");
      case SET_PRIORITYH_INSTR:
         return string("SET PRIORITY");
      case ADD_PRIORITY_INSTR:
         return string("ADD PRIORITY");
      case ADD_PRIORITYH_INSTR:
         return string("ADD PRIORITY HERE");
      case SET_DEFPRIOH_INSTR:
         return string("SET DEFAULT PRIORITY HERE");
      case SET_DEFPRIO_INSTR:
         return string("SET DEFAULT PRIORITY");
      case SET_STATIC_INSTR:
         return string("SET STATIC");
      case SET_STATICH_INSTR:
         return string("SET STATIC HERE");
      case SET_MOVING_INSTR:
         return string("SET MOVING");
      case SET_MOVINGH_INSTR:
         return string("SET MOVING HERE");
      case SET_AFFINITYH_INSTR:
         return string("SET AFFINITY HERE");
      case SET_AFFINITY_INSTR:
         return string("SET AFFINITY");
      case STOP_PROG_INSTR:
         return string("STOP PROGRAM");
      case CPU_ID_INSTR:
         return string("CPU ID");
      case NODE_PRIORITY_INSTR:
         return string("NODE PRIORITY");
      case JUMP_INSTR:
         return string("JUMP");
      case MVSTACKFIELD_INSTR:
         return string("MVSTACKFIELD");
      case CPU_STATIC_INSTR:
         return string("CPU STATIC");
      case MVCPUSREG_INSTR:
         return string("MOVE CPUS");
      case SET_CPUH_INSTR:
         return string("SET CPU HERE");
      case IS_STATIC_INSTR:
         return string("IS STATIC");
      case IS_MOVING_INSTR:
         return string("IS MOVING");
      case REM_PRIORITY_INSTR:
         return string("REMOVE PRIORITY");
      case FACTS_PROVED_INSTR:
         return string("FACTS PROVED");
      case FACTS_CONSUMED_INSTR:
         return string("FACTS CONSUMED");
      case FABS_INSTR:
         return string("FABS");
      case REMOTE_UPDATE_INSTR:
         return string("REMOTE UPDATE");
      case NODE_TYPE_INSTR:
         return string("NODE TYPE");
   }
   return string("");
}

pcounter instr_print(pcounter pc, const bool recurse, const int tabcount,
                     const program* prog, ostream& cout) {
   print_tab(tabcount);
   const string name(instr_name(fetch(pc)));
   cout << name;

   switch (fetch(pc)) {
      case RETURN_INSTR:
      case RETURN_LINEAR_INSTR:
      case RETURN_DERIVED_INSTR:
      case END_LINEAR_INSTR:
      case NEXT_INSTR:
      case PUSH_INSTR:
      case POP_INSTR:
      case PUSH_REGS_INSTR:
      case POP_REGS_INSTR:
      case RULE_DONE_INSTR:
      case REM_PRIORITYH_INSTR:
      case STOP_PROG_INSTR:
      case SET_STATICH_INSTR:
      case SET_MOVINGH_INSTR:
         cout << endl;
         break;
      case RESET_LINEAR_INSTR:
         cout << endl;
         if (recurse)
            pc = instrs_print_until_end_linear(advance(pc), tabcount + 1, prog,
                                               cout);
         break;
      case IF_INSTR: {
         cout << " (" << reg_string(if_reg(pc)) << ") THEN" << endl;
         if (recurse) {
            pcounter cont =
                instrs_print(advance(pc), if_jump(pc) - (advance(pc) - pc),
                             tabcount + 1, prog, cout);
            print_tab(tabcount);
            cout << "ENDIF" << endl;
            return cont;
         }
      } break;
      case IF_ELSE_INSTR: {
         cout << " (" << reg_string(if_reg(pc)) << ") THEN" << endl;
         if (recurse) {
            pcounter cont_then = instrs_print(
                advance(pc), if_else_jump_else(pc) - (advance(pc) - pc),
                tabcount + 1, prog, cout);
            (void)cont_then;
            print_tab(tabcount);
            cout << "ELSE" << endl;
            instrs_print(pc + if_else_jump_else(pc),
                         if_else_jump(pc) - if_else_jump_else(pc), tabcount + 1,
                         prog, cout);
            print_tab(tabcount);
            cout << "ENDIF" << endl;
            return pc + if_else_jump(pc);
         }
      } break;
      case TESTNIL_INSTR:
         cout << " " << reg_string(test_nil_op(pc)) << " TO "
              << reg_string(test_nil_dest(pc)) << endl;
         break;
      case ALLOC_INSTR:
         cout << " " << prog->get_predicate(alloc_predicate(pc))->get_name()
              << " TO " << reg_string(alloc_reg(pc)) << endl;
         break;
      case FLOAT_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case THREAD_SEND_INSTR:
      case SEND_INSTR:
         cout << " " << reg_string(send_msg(pc)) << " TO "
              << reg_string(send_dest(pc)) << endl;
         break;
      case ADDLINEAR_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case ADDPERS_INSTR:
      case ADDTPERS_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case RUNACTION_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case ENQUEUE_LINEAR_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case PERS_ITER_INSTR:
      case TPERS_ITER_INSTR:
      case LINEAR_ITER_INSTR:
      case TLINEAR_ITER_INSTR:
      case TRLINEAR_ITER_INSTR:
      case RLINEAR_ITER_INSTR:
         cout << " OVER " << prog->get_predicate(iter_predicate(pc))->get_name()
              << " MATCHING TO " << reg_string(iter_reg(pc)) << endl;
         print_iter_matches(pc, ITER_BASE, tabcount, prog);
         if (recurse) {
            instrs_print_until(pc + iter_inner_jump(pc),
                               pc + iter_outer_jump(pc), tabcount + 1, prog,
                               cout);
            return pc + iter_outer_jump(pc);
         }
         break;
      case OPERS_ITER_INSTR:
      case OLINEAR_ITER_INSTR:
      case ORLINEAR_ITER_INSTR:
         cout << " OVER " << prog->get_predicate(iter_predicate(pc))->get_name()
              << " (";
         {
            const byte opts(iter_options(pc));
            if (iter_options_random(opts))
               cout << "random";
            else if (iter_options_min(opts))
               cout << "min "
                    << iter_options_min_arg(iter_options_argument(pc));
         }
         cout << ") MATCHING TO " << reg_string(iter_reg(pc)) << endl;
         print_iter_matches(pc, OITER_BASE, tabcount, prog);
         if (recurse) {
            instrs_print_until(pc + iter_inner_jump(pc),
                               pc + iter_outer_jump(pc), tabcount + 1, prog,
                               cout);
            return pc + iter_outer_jump(pc);
         }
         break;
      case CALL0_INSTR:
         cout << call_string("0", pc, 0) << ")" << endl;
         break;
      case CALL1_INSTR:
         cout << call_string("1", pc, 1)
              << reg_string(pcounter_reg(pc + call_size)) << ")" << endl;
         break;
      case CALL2_INSTR:
         cout << call_string("2", pc, 2)
              << reg_string(pcounter_reg(pc + call_size)) << ", "
              << reg_string(pcounter_reg(pc + call_size + reg_val_size)) << ")"
              << endl;
         break;
      case CALL3_INSTR:
         cout << call_string("3", pc, 3)
              << reg_string(pcounter_reg(pc + call_size)) << ", "
              << reg_string(pcounter_reg(pc + call_size + reg_val_size)) << ", "
              << reg_string(pcounter_reg(pc + call_size + 2 * reg_val_size))
              << ")" << endl;
         break;
      case CALL_INSTR: {
         pcounter m = pc + CALL_BASE;

         cout << call_string("", pc, call_num_args(pc));

         for (size_t i = 0; i < call_num_args(pc); ++i) {
            if (i != 0) cout << ", ";

            cout << reg_string(pcounter_reg(m));
            m += reg_val_size;
         }
         cout << ")" << endl;
      } break;
      case CALLE_INSTR: {
         pcounter m = pc + CALL_BASE;

         cout << call_string("E", pc, calle_num_args(pc));

         for (size_t i = 0; i < calle_num_args(pc); ++i) {
            if (i != 0) cout << ", ";

            cout << reg_string(pcounter_reg(m));
            m += reg_val_size;
         }
         cout << ")" << endl;
      } break;
      case REMOVE_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case UPDATE_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case NOT_INSTR:
         cout << " " << reg_string(not_op(pc)) << " TO "
              << reg_string(not_dest(pc)) << endl;
         break;
      case LITERAL_CONS_INSTR: {
            pcounter m = pc + LITERAL_CONS_BASE;
            utils::byte id(literal_cons_type(pc));
            vm::type *t(prog->get_type(id));
            cout << " " << literal_cons_jump(pc) << " ";

            print_axiom_data(m, t);
            cout << endl;
         }
         break;
      case RETURN_SELECT_INSTR:
         cout << " " << return_select_jump(pc) << endl;
         break;
      case SELECT_INSTR:
         cout << endl;
         if (recurse) {
            const code_size_t elems(select_hash_size(pc));
            const pcounter hash_start(select_hash_start(pc));

            for (size_t i(0); i < elems; ++i) {
               print_tab(tabcount);
               cout << i << endl;

               const code_size_t hashed(select_hash(hash_start, i));

               if (hashed != 0)
                  instrs_print_until_return_select(
                      select_hash_code(hash_start, elems, hashed), tabcount + 1,
                      prog, cout);
            }

            return pc + select_size(pc);
         }
         break;
      case MARK_RULE_INSTR:
      case RULE_INSTR: {
         const size_t rule_id(rule_get_id(pc));

         cout << " " << rule_id << endl;
      } break;
      case NEW_NODE_INSTR:
         cout << " TO " << reg_string(new_node_reg(pc)) << endl;
         break;
      case NEW_AXIOMS_INSTR: {
         cout << endl;
         const pcounter end(pc + new_axioms_jump(pc));
         pcounter p(pc);
         p += NEW_AXIOMS_BASE;

         while (p < end) {
            // read axions until the end!
            const uint_val num(pcounter_int(p));
            pcounter_move_int(&p);
            predicate_id pid(predicate_get(p, 0));
            predicate* pred(prog->get_predicate(pid));
            p++;

            for(size_t j(0); j != num; ++j) {
               print_tab(tabcount + 1);
               if (pred->is_persistent_pred()) cout << "!";
               cout << pred->get_name() << "(";

               for (size_t i(0), num_fields(pred->num_fields()); i != num_fields;
                     ++i) {
                  type* t(pred->get_field_type(i));
                  print_axiom_data(p, t);

                  if (i != num_fields - 1) cout << ", ";
               }
               cout << ")" << endl;
            }
         }
      } break;
      case SEND_DELAY_INSTR:
         cout << " " << reg_string(send_delay_msg(pc)) << " TO "
              << reg_string(send_delay_dest(pc)) << " WITH "
              << send_delay_time(pc) << "ms" << endl;
         break;
      case PUSHN_INSTR:
         cout << " " << push_n(pc) << endl;
         break;
      case CALLF_INSTR: {
         const callf_id id(callf_get_id(pc));

         cout << " " << to_string((int)id) << endl;
      } break;
      case MAKE_STRUCTR_INSTR:
         cout << " TO " << reg_string(pcounter_reg(pc + instr_size + type_size))
              << endl;
         break;
      case MAKE_STRUCTF_INSTR:
         cout << " TO " << field_string(pc + instr_size) << endl;
         break;
      case STRUCT_VALRR_INSTR:
         cout << " " << struct_val_idx(pc) << " FROM "
              << reg_string(pcounter_reg(pc + instr_size + count_size))
              << " TO " << reg_string(pcounter_reg(pc + instr_size +
                                                   count_size + reg_val_size))
              << endl;
         break;
      case STRUCT_VALFR_INSTR:
         cout << " " << struct_val_idx(pc) << " FROM "
              << field_string(pc + instr_size + count_size) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + count_size +
                                         field_size)) << endl;
         break;
      case STRUCT_VALRF_INSTR:
         cout << " " << struct_val_idx(pc) << " FROM "
              << reg_string(pcounter_reg(pc + instr_size + count_size))
              << " TO "
              << field_string(pc + instr_size + count_size + reg_val_size)
              << endl;
         break;
      case STRUCT_VALRFR_INSTR:
         cout << " " << struct_val_idx(pc) << " FROM "
              << reg_string(pcounter_reg(pc + instr_size + count_size))
              << " (REFS)" << endl;
         break;
      case STRUCT_VALFF_INSTR:
         cout << " " << struct_val_idx(pc) << " FROM "
              << field_string(pc + instr_size + count_size) << " TO "
              << field_string(pc + instr_size + count_size + field_size)
              << endl;
         break;
      case STRUCT_VALFFR_INSTR:
         cout << " " << struct_val_idx(pc) << " FROM "
              << field_string(pc + instr_size + count_size) << " TO "
              << field_string(pc + instr_size + count_size + field_size)
              << " (REFS)" << endl;
         break;
      case MVINTFIELD_INSTR: {
         const int_val i(pcounter_int(pc + instr_size));
         const string field(field_string(pc + instr_size + int_size));
         cout << " " << int_string(i) << " TO " << field << endl;
      } break;
      case MVFIELDFIELD_INSTR: {
         const string field1(field_string(pc + instr_size));
         const string field2(field_string(pc + instr_size + field_size));
         cout << " " << field1 << " TO " << field2 << endl;
      } break;
      case MVFIELDFIELDR_INSTR: {
         const string field1(field_string(pc + instr_size));
         const string field2(field_string(pc + instr_size + field_size));
         cout << " " << field1 << " TO " << field2 << " (REFS)" << endl;
      } break;
      case MVTYPEREG_INSTR:
      case MVINTREG_INSTR: {
         const int_val i(pcounter_int(pc + instr_size));
         const reg_num reg(pcounter_reg(pc + instr_size + int_size));

         cout << " " << int_string(i) << " TO " << reg_string(reg) << endl;
      } break;
      case MVFIELDREG_INSTR: {
         const string field(field_string(pc + instr_size));
         const reg_num reg(pcounter_reg(pc + instr_size + field_size));

         cout << " " << field << " TO " << reg_string(reg) << endl;
      } break;
      case MVPTRREG_INSTR: {
         const ptr_val p(pcounter_ptr(pc + instr_size));
         const reg_num reg(pcounter_reg(pc + instr_size + ptr_size));

         cout << " " << ptr_string(p) << " TO " << reg_string(reg) << endl;
      } break;
      case MVNILFIELD_INSTR:
         cout << " TO " << field_string(pc + instr_size) << endl;
         break;
      case MVNILREG_INSTR:
         cout << " TO " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case MVREGFIELD_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << field_string(pc + instr_size + reg_val_size) << endl;
         break;
      case MVREGFIELDR_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << field_string(pc + instr_size + reg_val_size) << " (REFS)"
              << endl;
         break;
      case MVTHREADIDFIELD_INSTR:
      case MVHOSTFIELD_INSTR:
         cout << " TO " << field_string(pc + instr_size) << endl;
         break;
      case MVREGCONST_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << const_id_string(
                     pcounter_const_id(pc + instr_size + reg_val_size)) << endl;
         break;
      case MVCONSTFIELD_INSTR:
         cout << " " << const_id_string(pcounter_const_id(pc + instr_size))
              << " TO " << field_string(pc + instr_size + const_id_size)
              << endl;
         break;
      case MVCONSTFIELDR_INSTR:
         cout << " " << const_id_string(pcounter_const_id(pc + instr_size))
              << " TO " << field_string(pc + instr_size + const_id_size)
              << " (REFS)" << endl;
         break;
      case MVADDRFIELD_INSTR:
         cout << " " << node_string(pcounter_node(pc + instr_size)) << " TO "
              << field_string(pc + instr_size + node_size) << endl;
         break;
      case MVFLOATFIELD_INSTR:
         cout << " " << pcounter_float(pc + instr_size) << " TO "
              << field_string(pc + instr_size + float_size) << endl;
         break;
      case MVFLOATREG_INSTR:
         cout << " " << float_string(pcounter_float(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + float_size)) << endl;
         break;
      case MVINTCONST_INSTR:
         cout << " " << int_string(pcounter_int(pc + instr_size)) << " TO "
              << const_id_string(pcounter_const_id(pc + instr_size + int_size))
              << endl;
         break;
      case MVWORLDFIELD_INSTR:
         cout << " TO " << field_string(pc + instr_size) << endl;
         break;
      case MVSTACKPCOUNTER_INSTR:
         cout << " TO " << stack_string(pcounter_stack(pc + instr_size))
              << endl;
         break;
      case MVPCOUNTERSTACK_INSTR:
         cout << " TO " << stack_string(pcounter_stack(pc + instr_size))
              << endl;
         break;
      case MVSTACKREG_INSTR:
         cout << " " << stack_string(pcounter_stack(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + stack_val_size))
              << endl;
         break;
      case MVREGSTACK_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << stack_string(pcounter_stack(pc + instr_size + reg_val_size))
              << endl;
         break;
      case MVADDRREG_INSTR:
         cout << " " << node_string(pcounter_node(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + node_size)) << endl;
         break;
      case MVTHREADIDREG_INSTR:
      case MVHOSTREG_INSTR:
         cout << " TO " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case MVINTSTACK_INSTR:
         cout << " " << int_string(pcounter_int(pc + instr_size)) << " TO "
              << stack_string(pcounter_stack(pc + instr_size + int_size))
              << endl;
         break;
      case MVARGREG_INSTR:
         cout << " " << argument_string(pcounter_argument_id(pc + instr_size))
              << " TO "
              << reg_string(pcounter_reg(pc + instr_size + argument_size))
              << endl;
         break;
      case ADDRNOTEQUAL_INSTR:
         cout << operation_string(pc, "ADDR NOT EQUAL") << endl;
         break;
      case ADDREQUAL_INSTR:
         cout << operation_string(pc, "ADDR EQUAL") << endl;
         break;
      case INTMINUS_INSTR:
         cout << operation_string(pc, "INT MINUS") << endl;
         break;
      case INTEQUAL_INSTR:
         cout << operation_string(pc, "INT EQUAL") << endl;
         break;
      case INTNOTEQUAL_INSTR:
         cout << operation_string(pc, "INT NOT EQUAL") << endl;
         break;
      case INTPLUS_INSTR:
         cout << operation_string(pc, "INT PLUS") << endl;
         break;
      case INTLESSER_INSTR:
         cout << operation_string(pc, "INT LESSER") << endl;
         break;
      case INTGREATEREQUAL_INSTR:
         cout << operation_string(pc, "INT GREATER EQUAL") << endl;
         break;
      case BOOLOR_INSTR:
         cout << operation_string(pc, "BOOL OR") << endl;
         break;
      case INTLESSEREQUAL_INSTR:
         cout << operation_string(pc, "INT LESSER EQUAL") << endl;
         break;
      case INTGREATER_INSTR:
         cout << operation_string(pc, "INT GREATER") << endl;
         break;
      case INTMUL_INSTR:
         cout << operation_string(pc, "INT MUL") << endl;
         break;
      case INTDIV_INSTR:
         cout << operation_string(pc, "INT DIV") << endl;
         break;
      case INTMOD_INSTR:
         cout << operation_string(pc, "INT MOD") << endl;
         break;
      case FLOATPLUS_INSTR:
         cout << operation_string(pc, "FLOAT PLUS") << endl;
         break;
      case FLOATMINUS_INSTR:
         cout << operation_string(pc, "FLOAT MINUS") << endl;
         break;
      case FLOATMUL_INSTR:
         cout << operation_string(pc, "FLOAT MUL") << endl;
         break;
      case FLOATDIV_INSTR:
         cout << operation_string(pc, "FLOAT DIV") << endl;
         break;
      case FLOATEQUAL_INSTR:
         cout << operation_string(pc, "FLOAT EQUAL") << endl;
         break;
      case FLOATNOTEQUAL_INSTR:
         cout << operation_string(pc, "FLOAT NOT EQUAL") << endl;
         break;
      case FLOATLESSER_INSTR:
         cout << operation_string(pc, "FLOAT LESSER") << endl;
         break;
      case FLOATLESSEREQUAL_INSTR:
         cout << operation_string(pc, "FLOAT LESSER EQUAL") << endl;
         break;
      case FLOATGREATER_INSTR:
         cout << operation_string(pc, "FLOAT GREATER") << endl;
         break;
      case FLOATGREATEREQUAL_INSTR:
         cout << operation_string(pc, "FLOAT GREATER EQUAL") << endl;
         break;
      case MVREGREG_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case BOOLEQUAL_INSTR:
         cout << operation_string(pc, "BOOL EQUAL") << endl;
         break;
      case BOOLNOTEQUAL_INSTR:
         cout << operation_string(pc, "BOOL NOT EQUAL") << endl;
         break;
      case HEADRR_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case HEADFR_INSTR:
         cout << " " << field_string(pc + instr_size) + " TO "
              << reg_string(pcounter_reg(pc + instr_size + field_size)) << endl;
         break;
      case HEADFF_INSTR:
         cout << " " << field_string(pc + instr_size) << " TO "
              << field_string(pc + instr_size + field_size) << endl;
         break;
      case HEADRF_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << field_string(pc + instr_size + reg_val_size) << endl;
         break;
      case HEADFFR_INSTR:
         cout << " " << field_string(pc + instr_size) << " TO "
              << field_string(pc + instr_size + field_size) << " (REFS)"
              << endl;
         break;
      case HEADRFR_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << field_string(pc + instr_size + reg_val_size) << " (REFS)"
              << endl;
         break;
      case TAILRR_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case TAILFR_INSTR:
         cout << " " << field_string(pc + instr_size) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + field_size)) << endl;
         break;
      case TAILFF_INSTR:
         cout << " " << field_string(pc + instr_size) << " TO "
              << field_string(pc + instr_size + field_size) << endl;
         break;
      case TAILRF_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << field_string(pc + instr_size + reg_val_size) << endl;
         break;
      case MVWORLDREG_INSTR:
         cout << " TO " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case MVCONSTREG_INSTR:
         cout << " " << const_id_string(pcounter_const_id(pc + instr_size))
              << " TO "
              << reg_string(pcounter_reg(pc + instr_size + const_id_size))
              << endl;
         break;
      case CONSRRR_INSTR:
         cout << " (" << reg_string(pcounter_reg(pc + instr_size + type_size))
              << "::" << reg_string(pcounter_reg(pc + instr_size + type_size +
                                                 reg_val_size)) << ") TO "
              << reg_string(pcounter_reg(pc + instr_size + type_size +
                                         2 * reg_val_size)) << " GC "
              << bool_string(pcounter_bool(pc + instr_size + type_size +
                                           3 * reg_val_size)) << endl;
         break;
      case CONSRFF_INSTR:
         cout << " (" << reg_string(pcounter_reg(pc + instr_size))
              << "::" << field_string(pc + instr_size + reg_val_size) << ") TO "
              << field_string(pc + instr_size + reg_val_size + field_size)
              << endl;
         break;
      case CONSFRF_INSTR:
         cout << " (" << field_string(pc + instr_size)
              << "::" << reg_string(pcounter_reg(pc + instr_size + field_size))
              << ") TO "
              << field_string(pc + instr_size + field_size + reg_val_size)
              << endl;
         break;
      case CONSFFR_INSTR:
         cout << " (" << field_string(pc + instr_size)
              << "::" << field_string(pc + instr_size + field_size) << ") TO "
              << reg_string(pcounter_reg(pc + instr_size + 2 * field_size))
              << " GC "
              << bool_string(pcounter_bool(pc + instr_size + 2 * field_size +
                                           reg_val_size)) << endl;
         break;
      case CONSRRF_INSTR:
         cout << " (" << reg_string(pcounter_reg(pc + instr_size)) << "::"
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << ") TO " << field_string(pc + instr_size + 2 * reg_val_size)
              << endl;
         break;
      case CONSRFR_INSTR:
         cout << " (" << reg_string(pcounter_reg(pc + instr_size))
              << "::" << field_string(pc + instr_size + reg_val_size) << ") TO "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size +
                                         field_size)) << " GC "
              << bool_string(pcounter_bool(pc + instr_size + reg_val_size +
                                           field_size + reg_val_size)) << endl;
         break;
      case CONSFRR_INSTR:
         cout << " (" << field_string(pc + instr_size + type_size)
              << "::" << reg_string(pcounter_reg(pc + instr_size + type_size +
                                                 field_size)) << ") TO "
              << reg_string(pcounter_reg(pc + instr_size + type_size +
                                         field_size + reg_val_size)) << " GC "
              << bool_string(pcounter_bool(pc + instr_size + type_size +
                                           field_size + 2 * reg_val_size))
              << endl;
         break;
      case CONSFFF_INSTR:
         cout << " (" << field_string(pc + instr_size)
              << "::" << field_string(pc + instr_size + field_size) << ") TO "
              << field_string(pc + instr_size + 2 * field_size) << endl;
         break;
      case MVFLOATSTACK_INSTR:
         cout << " " << float_string(pcounter_float(pc + instr_size)) << " TO "
              << stack_string(pcounter_stack(pc + instr_size + float_size))
              << endl;
         break;
      case SET_PRIORITY_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " ON "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case SET_PRIORITYH_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case ADD_PRIORITY_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " ON "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case ADD_PRIORITYH_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case SET_DEFPRIOH_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case SET_DEFPRIO_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " ON "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case SET_STATIC_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case SET_MOVING_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case SET_AFFINITYH_INSTR:
         cout << " TO " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case SET_AFFINITY_INSTR:
         cout << " OF "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << " TO " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case CPU_ID_INSTR:
         cout << " OF " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case NODE_PRIORITY_INSTR:
         cout << " OF " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case JUMP_INSTR:
         cout << " TO " << jump_get(pc, instr_size) << endl;
         break;
      case MVSTACKFIELD_INSTR:
         cout << " " << stack_string(pcounter_stack(pc + instr_size)) << " TO "
              << field_string(pc + instr_size + stack_val_size) << endl;
         break;
      case CPU_STATIC_INSTR:
         cout << " OF " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case MVCPUSREG_INSTR:
         cout << " TO " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case SET_CPUH_INSTR:
         cout << " TO " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case IS_STATIC_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case IS_MOVING_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case REM_PRIORITY_INSTR:
         cout << " FROM " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case FACTS_PROVED_INSTR:
         cout << " OF " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case FACTS_CONSUMED_INSTR:
         cout << " OF " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case SCHEDULE_NEXT_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << endl;
         break;
      case FABS_INSTR:
         cout << " " << reg_string(pcounter_reg(pc + instr_size)) << " TO "
              << reg_string(pcounter_reg(pc + instr_size + reg_val_size))
              << endl;
         break;
      case REMOTE_UPDATE_INSTR: {
         const predicate_id edit(remote_update_edit(pc));
         const predicate_id target(remote_update_target(pc));
         cout << " TO " << reg_string(remote_update_dest(pc)) << " OF"
              << " " << theProgram->get_predicate(target)->get_name()
              << " FROM " << theProgram->get_predicate(edit)->get_name() << " ";
         const size_t nregs(remote_update_nregs(pc));
         for (size_t i(0); i < nregs; ++i)
            cout << reg_string(pcounter_reg(pc + REMOTE_UPDATE_BASE +
                                            i * reg_val_size)) << " ";
         cout << "(common: " << remote_update_common(pc) << ")" << endl;
      } break;
      case NODE_TYPE_INSTR:
         cout << " TO " << reg_string(node_type_reg(pc)) << " FROM " << node_type_nodes(pc) << " NODES" << endl;
         break;
      default:
         throw malformed_instr_error("unknown instruction code");
   }

   return advance(pc);
}

pcounter instr_print_simple(pcounter pc, const int tabcount,
                            const program* prog, ostream& cout) {
   return instr_print(pc, false, tabcount, prog, cout);
}

byte_code instrs_print(byte_code code, const code_size_t len,
                       const int tabcount, const program* prog, ostream& cout) {
   pcounter pc = code;
   pcounter until = code + len;

   for (; pc < until;) {
      pc = instr_print(pc, true, tabcount, prog, cout);
   }

   return until;
}
}
