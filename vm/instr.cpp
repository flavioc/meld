
#include "vm/predicate.hpp"
#include "vm/instr.hpp"
#include "vm/program.hpp"
#include "utils/utils.hpp"

using namespace std;
using namespace vm;
using namespace utils;

namespace vm {
   
namespace instr {
   
string
op_string(const instr_op op)
{
   switch(op) {
		case OP_NEQI: return string("INT NOT EQUAL");
		case OP_EQI: return string("INT EQUAL"); 
		case OP_LESSI: return string("INT LESSER"); 
		case OP_LESSEQI: return string("INT LESSER EQUAL"); 
		case OP_GREATERI: return string("INT GREATER"); 
		case OP_GREATEREQI: return string("INT GREATER EQUAL"); 
		case OP_MODI: return string("INT MOD"); 
		case OP_PLUSI: return string("INT PLUS"); 
		case OP_MINUSI: return string("INT MINUS"); 
		case OP_TIMESI: return string("INT TIMES"); 
		case OP_DIVI: return string("INT DIV"); 
		case OP_NEQF: return string("FLOAT NOT EQUAL"); 
		case OP_EQF: return string("FLOAT EQUAL"); 
		case OP_LESSF: return string("FLOAT LESSER"); 
		case OP_LESSEQF: return string("FLOAT LESSER EQUAL"); 
		case OP_GREATERF: return string("FLOAT GREATER"); 
		case OP_GREATEREQF: return string("FLOAT GREATER EQUAL"); 
		case OP_MODF: return string("FLOAT MOD"); 
		case OP_PLUSF: return string("FLOAT PLUS"); 
		case OP_MINUSF: return string("FLOAT MINUS"); 
		case OP_TIMESF: return string("FLOAT TIMES"); 
		case OP_DIVF: return string("FLOAT DIV"); 
		case OP_NEQA: return string("ADDR NOT EQUAL"); 
		case OP_EQA: return string("ADDR EQUAL"); 
      case OP_GREATERA: return string("ADDR GREATER");
      case OP_ORB: return string("BOOL OR");
	}
	
   return string("");
}

static inline string
reg_string(const reg_num num)
{
   return string("reg ") + to_string((int)num);
}

string
val_string(const instr_val v, pcounter *pm, const program *prog)
{
   if(val_is_tuple(v))
      return string("tuple");
   else if(val_is_reg(v))
      return reg_string(val_reg(v));
   else if(val_is_host(v))
      return string("host");
   else if(val_is_nil(v))
      return string("nil");
   else if(val_is_field(v)) {
      const string ret(to_string((int)val_field_reg(*pm)) +
         string(".") + to_string((int)val_field_num(*pm)));
      pcounter_move_field(pm);
      return ret;
   } else if(val_is_int(v)) {
      const string ret(to_string(pcounter_int(*pm)));
      pcounter_move_int(pm);
      return string("INT ") + ret;
   } else if(val_is_bool(v)) {
      const bool_val v = pcounter_bool(*pm);
      pcounter_move_bool(pm);
      return string("BOOL ") + (v ? "true" : "false");
   } else if(val_is_float(v)) {
      const string ret(to_string(pcounter_float(*pm)));
      pcounter_move_float(pm);
      return string("FLOAT ") + ret;
   } else if(val_is_node(v)) {
      const string ret(string("@") + to_string(pcounter_node(*pm)));
      pcounter_move_node(pm);
      return ret;
	} else if(val_is_string(v)) {
      const uint_val id(pcounter_uint(*pm));
      runtime::rstring::ptr p(prog->get_default_string(id));

      assert(p != NULL);
		const string ret(string("\"") + p->get_content() + "\"");
		pcounter_move_uint(pm);
		return ret;
	} else if(val_is_arg(v)) {
		const string ret(string("ARG ") + to_string(pcounter_argument_id(*pm)));

		pcounter_move_argument_id(pm);

		return ret;
   } else if(val_is_stack(v)) {
      const offset_num offset(pcounter_offset_num(*pm));

      pcounter_move_offset_num(pm);

      return string("STACK ") + to_string((int)offset);
   } else if(val_is_pcounter(v)) {
      return string("PCOUNTER");
	} else if(val_is_const(v)) {
		const string ret(string("CONST ") + to_string(pcounter_const_id(*pm)));
		
		pcounter_move_const_id(pm);
		
		return ret;
   } else if(val_is_ptr(v)) {
      const string ret(string("PTR ") + to_string(pcounter_ptr(*pm)));

      pcounter_move_ptr(pm);

      return ret;
   } else
		throw type_error("Unrecognized val type " + to_string(v) + " (val_string)");
   
   return string("");
}

}

using namespace vm::instr;

static inline void
instrs_print_until_return_select(byte_code code, const int tabcount, const program* prog, ostream& cout)
{
   pcounter pc = code;
   pcounter new_pc;
   
	while (true) {
		new_pc = instr_print(pc, true, tabcount, prog, cout);
		if(fetch(pc) == RETURN_SELECT_INSTR)
         return;
      pc = new_pc;
	}
}

static inline pcounter
instrs_print_until_end_linear(byte_code code, const int tabcount, const program* prog, ostream& cout)
{
   pcounter pc = code;
   pcounter new_pc;

   while (true) {
      new_pc = instr_print(pc, true, tabcount, prog, cout);
      if(fetch(pc) == END_LINEAR_INSTR)
         return pc;
      pc = new_pc;
   }
}

static inline pcounter
instrs_print_until(byte_code code, byte_code until, const int tabcount, const program* prog, ostream& cout)
{
   pcounter pc = code;
   
	for (; pc < until; ) {
		pc = instr_print(pc, true, tabcount, prog, cout);
	}
	
   return until;
}

static inline void
print_tab(const int tabcount)
{
   for(int i = 0; i < tabcount; ++i)
      cout << "  ";
}

static inline void
print_axiom_data(pcounter& p, type *t, bool in_list = false)
{
   switch(t->get_type()) {
      case FIELD_INT:
         cout << pcounter_int(p);
         pcounter_move_int(&p);
         break;
      case FIELD_FLOAT:
         cout << pcounter_float(p);
         pcounter_move_float(&p);
         break;
      case FIELD_NODE:
         cout << "@" << pcounter_node(p);
         pcounter_move_node(&p);
         break;
      case FIELD_LIST: {
         if(!in_list) {
            cout << "[";
         }
         if(*p++ == 0) {
            cout << "]";
            break;
         }
         list_type *lt((list_type*)t);
         print_axiom_data(p, lt->get_subtype());
         if(*p == 1)
            cout << ", ";
         print_axiom_data(p, lt, true);
        }
         break;
      default: assert(false);
   }
}

pcounter
instr_print(pcounter pc, const bool recurse, const int tabcount, const program *prog, ostream& cout)
{
   print_tab(tabcount);

   switch(fetch(pc)) {
      case RETURN_INSTR:
         cout << "RETURN" << endl;
			break;
      case RETURN_LINEAR_INSTR:
         cout << "RETURN LINEAR" << endl;
         break;
      case RETURN_DERIVED_INSTR:
         cout << "RETURN DERIVED" << endl;
         break;
      case END_LINEAR_INSTR:
         cout << "END LINEAR" << endl;
         break;
      case RESET_LINEAR_INSTR:
         cout << "RESET LINEAR" << endl;
         pc = instrs_print_until_end_linear(advance(pc), tabcount + 1, prog, cout);
         break;
	   case IF_INSTR: {
            cout << "IF (" << reg_string(if_reg(pc)) << ") THEN" << endl;
				if(recurse) {
               pcounter cont = instrs_print(advance(pc), if_jump(pc) - (advance(pc) - pc),
                                       tabcount + 1, prog, cout);
               print_tab(tabcount);
               cout << "ENDIF" << endl;
					return cont;
				}
			}
			break;
		case TEST_NIL_INSTR: {
				pcounter m = pc + TEST_NIL_BASE;
            const string op(val_string(test_nil_op(pc), &m, prog));
            const string dest(val_string(test_nil_dest(pc), &m, prog));

            cout << "TEST-NIL " << op << " TO " << dest << endl;
			}
			break;
      case MOVE_INSTR: {
				pcounter m = pc + MOVE_BASE;
            const string from(val_string(move_from(pc), &m, prog));
            const string to(val_string(move_to(pc), &m, prog));

            cout << "MOVE " << from << " TO " << to << endl;
		   }
		   break;
   	case MOVE_NIL_INSTR: {
   			pcounter m = pc + MOVE_NIL_BASE;

            cout << "MOVE-NIL TO "
                 << val_string(move_nil_dest(pc), &m, prog)
                 << endl;
   		}
   		break;
   	case ALLOC_INSTR:
         cout << "ALLOC " << prog->get_predicate(alloc_predicate(pc))->get_name()
              << " TO " << reg_string(alloc_reg(pc))
              << endl;
   		break;
   	case OP_INSTR: {
   			pcounter m = pc + OP_BASE;
            const string arg1(val_string(op_arg1(pc), &m, prog));
            const string arg2(val_string(op_arg2(pc), &m, prog));
            const string dest(val_string(op_dest(pc), &m, prog));

            cout << "OP " << arg1 << " " << op_string(op_op(pc))
                 << " " << arg2 << " TO " << dest
                 << endl;
   		 }
   		break;
   	case FLOAT_INSTR: {
            pcounter m = pc + FLOAT_BASE;
            const string op(val_string(float_op(pc), &m, prog));
            const string dest(val_string(float_dest(pc), &m, prog));
            
   	      cout << "FLOAT " << op
                 << " TO " << dest << endl;
           }
           break;
      case SEND_INSTR:
         cout << "SEND " << reg_string(send_msg(pc))
              << " TO " << reg_string(send_dest(pc))
              << endl;
   		break;
   	case ITER_INSTR: {
            pcounter m = pc + ITER_BASE;
				const byte opts(iter_options(pc));
            
            cout << "ITERATE OVER " << prog->get_predicate(iter_predicate(pc))->get_name() << " (";

				if(iter_options_random(opts))
					cout << "r";
				if(iter_options_to_delete(opts))
					cout << "d";
				if(iter_options_min(opts))
					cout << "m" << iter_options_min_arg(iter_options_argument(pc));

				cout << ") MATCHING";
            
            if(!iter_match_none(m)) {
               while (true) {
                  iter_match match = m;
                  
                  m += iter_match_size;
                  
                  cout << endl;
                  print_tab(tabcount+1);
                  cout << "  (match)." << iter_match_field(match)
                       << "=" << val_string(iter_match_val(match), &m, prog);
                  if(iter_match_end(match))
                     break;
               }
            }
            
            cout << endl;

            if(recurse) {
               instrs_print_until(advance(pc), pc + iter_jump(pc), tabcount + 1, prog, cout);
               return pc + iter_jump(pc);
            }
   		}
   		break;
   	case NEXT_INSTR:
         cout << "NEXT" << endl;
         break;
   	case CALL_INSTR: {
            pcounter m = pc + CALL_BASE;
            const external_function_id id(call_extern_id(pc));

   	      cout << "CALL func(" << id << "):"
   	           << call_num_args(pc) << " TO "
                 << reg_string(call_dest(pc)) << " = (";
            
            for(size_t i = 0; i < call_num_args(pc); ++i) {
               if(i != 0)
                  cout << ", ";
               
               pcounter val_ptr(m);
               m += val_size;
               cout << val_string(call_val(val_ptr), &m, prog);
            }
            cout << ")" << endl;
   		}
   		break;
   	case CALLE_INSTR: {
            pcounter m = pc + CALL_BASE;
            const external_function_id id(calle_extern_id(pc));

   	      cout << "CALLE func(" << id << "):"
   	           << calle_num_args(pc) << " TO "
                 << reg_string(calle_dest(pc)) << " = (";
            
            for(size_t i = 0; i < calle_num_args(pc); ++i) {
               if(i != 0)
                  cout << ", ";
               
               pcounter val_ptr(m);
               m += val_size;
               cout << val_string(calle_val(val_ptr), &m, prog);
            }
            cout << ")" << endl;
   		}
   		break;
      case DELETE_INSTR: {
            pcounter m = pc + DELETE_BASE;
            const predicate_id pred_id(delete_predicate(pc));
            const predicate *pred(prog->get_predicate(pred_id));
            const size_t num_args(delete_num_args(pc));
            
            cout << "DELETE " << pred->get_name()
                 << " USING ";
                 
            for(size_t i(0); i < num_args; ++i) {
               if(i != 0)
                  cout << ", ";
               
               pcounter val_ptr(m);
               cout << (int)delete_index(val_ptr) << ":";
               
               m += index_size + val_size; // skip index and value bytes of this arg
               
               cout << val_string(delete_val(val_ptr), &m, prog);
            }
            cout << endl; 
         }
         break;
      case REMOVE_INSTR: {
            cout << "REMOVE " << reg_string(remove_source(pc)) << endl;
         }
         break;
   	case CONS_INSTR: {
   			pcounter m = pc + CONS_BASE;
            const size_t type_id(cons_type(pc));
            list_type *lt((list_type*)prog->get_type(type_id));
            const string head(val_string(cons_head(pc), &m, prog));
            const string tail(val_string(cons_tail(pc), &m, prog));
            const string dest(val_string(cons_dest(pc), &m, prog));
   			
            cout << "CONS (" << head << "::" << tail << ") " << lt->string() << " TO " << dest << endl;
   		}
   		break;
   	case HEAD_INSTR: {
   			pcounter m = pc + HEAD_BASE;
            const string cons(val_string(head_cons(pc), &m, prog));
            const string dest(val_string(head_dest(pc), &m, prog));

            cout << "HEAD " << cons << " TO " << dest << endl;
   	   }
   	   break;
    	case TAIL_INSTR: {
    			pcounter m = pc + TAIL_BASE;
            const string cons(val_string(tail_cons(pc), &m, prog));
            const string dest(val_string(tail_dest(pc), &m, prog));
    			
            cout << "TAIL " << cons << " TO " << dest << endl;
    		}
    		break;
    	case NOT_INSTR: {
    			pcounter m = pc + NOT_BASE;
            const string op(val_string(not_op(pc), &m, prog));
            const string dest(val_string(not_dest(pc), &m, prog));

            cout << "NOT " << op << " TO " << dest << endl;
    		}
    	   break;
    	case RETURN_SELECT_INSTR:
         cout << "RETURN SELECT " << return_select_jump(pc) << endl;
         break;
      case SELECT_INSTR:
         cout << "SELECT BY NODE" << endl;
         if(recurse) {
            const code_size_t elems(select_hash_size(pc));
            const pcounter hash_start(select_hash_start(pc));
            
            for(size_t i(0); i < elems; ++i) {
               print_tab(tabcount);
               cout << i << endl;
               
               const code_size_t hashed(select_hash(hash_start, i));
               
               if(hashed != 0)
                  instrs_print_until_return_select(select_hash_code(hash_start, elems, hashed), tabcount + 1, prog, cout);
            }
            
            return pc + select_size(pc);
         }
         break;
      case COLOCATED_INSTR: {
            pcounter m = pc + COLOCATED_BASE;
            const string first(val_string(colocated_first(pc), &m, prog));
            const string second(val_string(colocated_second(pc), &m, prog));
         
            cout << "COLOCATED (" << first << ", " << second
               << ") TO " << reg_string(colocated_dest(pc)) << endl;
            
         }
         break;
      case RULE_INSTR: {
            const size_t rule_id(rule_get_id(pc));

            cout << "RULE " << rule_id << endl;
        }
        break;
      case RULE_DONE_INSTR:
         cout << "RULE DONE" << endl;
         break;

      case NEW_NODE_INSTR:
         cout << "NEW NODE TO " << reg_string(new_node_reg(pc)) << endl;
         break;

      case NEW_AXIOMS_INSTR: {
         cout << "NEW AXIOMS" << endl;
         const pcounter end(pc + new_axioms_jump(pc));
         pcounter p(pc);
         p += NEW_AXIOMS_BASE;

         while(p < end) {
            // read axions until the end!
            predicate_id pid(predicate_get(p, 0));
            predicate *pred(prog->get_predicate(pid));
            print_tab(tabcount+1);
            cout << pred->get_name() << "(";

            p++;

            for(size_t i(0), num_fields(pred->num_fields());
                  i != num_fields;
                  ++i)
            {
               type *t(pred->get_field_type(i));
               print_axiom_data(p, t);

               if(i != num_fields-1)
                  cout << ", ";
            }
            cout << ")" << endl;
         }
                             }
         break;

      case SEND_DELAY_INSTR:
         cout << "SEND " << reg_string(send_delay_msg(pc))
              << " TO " << reg_string(send_delay_dest(pc))
              << " WITH DELAY " << send_delay_time(pc) << "ms"
              << endl;

         break;

      case PUSH_INSTR:
         cout << "PUSH" << endl;
         break;

      case POP_INSTR:
         cout << "POP" << endl;
         break;

      case PUSH_REGS_INSTR:
         cout << "PUSH REGS" << endl;
         break;

      case POP_REGS_INSTR:
         cout << "POP REGS" << endl;
         break;

      case CALLF_INSTR: {
         const callf_id id(callf_get_id(pc));

         cout << "CALLF " << to_string((int)id) << endl;
         break;
      }

      case MAKE_STRUCT_INSTR: {
         const size_t type_id(make_struct_type(pc));
         struct_type *st((struct_type*)prog->get_type(type_id));
         const instr_val to(make_struct_to(pc));
         pcounter m = pc + MAKE_STRUCT_BASE;

         cout << "MAKE STRUCT " << st->string() << " TO " << val_string(to, &m, prog) << endl;
      }
      break;

      case STRUCT_VAL_INSTR: {
         const size_t idx(struct_val_idx(pc));
         const instr_val from(struct_val_from(pc));
         const instr_val to(struct_val_to(pc));
         pcounter m = pc + STRUCT_VAL_BASE;

         cout << "STRUCT VAL " << idx << " FROM " << val_string(from, &m, prog)
            << " TO " << val_string(to, &m, prog) << endl;
      }
      break;

    	case ELSE_INSTR:
		default:
         throw malformed_instr_error("unknown instruction code");
	}
	
   return advance(pc);
}

pcounter
instr_print_simple(pcounter pc, const int tabcount, const program *prog, ostream& cout)
{
   return instr_print(pc, false, tabcount, prog, cout);
}
 
byte_code
instrs_print(byte_code code, const code_size_t len, const int tabcount, const program* prog, ostream& cout)
{
   pcounter pc = code;
   pcounter until = code + len;
   
	for (; pc < until; ) {
		pc = instr_print(pc, true, tabcount, prog, cout);
	}
	
   return until;
}
  
}
