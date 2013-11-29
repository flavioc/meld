
#include <iostream>
#include <algorithm>
#include <functional>
#include <cmath>
#include <sstream>

#include "vm/exec.hpp"
#include "vm/tuple.hpp"
#include "vm/match.hpp"
#include "db/tuple.hpp"
#include "process/machine.hpp"
#ifdef USE_UI
#include "ui/manager.hpp"
#endif

//#define DEBUG_SENDS
//#define DEBUG_INSTRS
//#define DEBUG_RULES
//#define DEBUG_REMOVE

#if defined(DEBUG_SENDS)
static boost::mutex print_mtx;
#endif

using namespace vm;
using namespace vm::instr;
using namespace std;
using namespace db;
using namespace runtime;
using namespace utils;

namespace vm
{
   
enum return_type {
   RETURN_OK,
   RETURN_SELECT,
   RETURN_NEXT,
   RETURN_LINEAR,
   RETURN_DERIVED,
	RETURN_END_LINEAR,
   RETURN_NO_RETURN
};

static inline return_type execute(pcounter, state&);

static inline node_val
get_node_val(pcounter& m, state& state)
{
   (void)state;

   const node_val ret(pcounter_node(m));
   
   pcounter_move_node(&m);

   assert(ret <= state.all->DATABASE->max_id());
   
   return ret;
}

static inline node_val
get_node_val(const pcounter& m, state& state)
{
   (void)state;
   return pcounter_node(m);
}

static inline void
move_to_reg(const pcounter& m, state& state,
   const reg_num& reg, const instr_val& from)
{
   if(val_is_float(from))
      state.set_float(reg, pcounter_float(m));
   else if(val_is_int(from))
      state.set_int(reg, pcounter_int(m));
   else if(val_is_field(from)) {
      const field_num field(val_field_num(m));
      const reg_num from_reg(val_field_reg(m));
      const tuple *tuple(state.get_tuple(from_reg));

      switch(tuple->get_field_type(field)->get_type()) {
         case FIELD_INT: state.set_int(reg, tuple->get_int(field)); break;
         case FIELD_FLOAT: state.set_float(reg, tuple->get_float(field)); break;
         case FIELD_LIST: state.set_cons(reg, tuple->get_cons(field)); break;
         case FIELD_NODE: state.set_node(reg, tuple->get_node(field)); break;
			case FIELD_STRING: state.set_string(reg, tuple->get_string(field)); break;
         default: throw vm_exec_error("don't know how to move this field (move_to_reg)");
      }
      
   } else if(val_is_host(from))
      state.set_node(reg, state.node->get_id());
   else if(val_is_node(from))
      state.set_node(reg, get_node_val(m, state));
   else if(val_is_reg(from))
      state.copy_reg(val_reg(from), reg);
   else if(val_is_tuple(from)) {
		if(state.tuple_leaf != NULL)
      	state.set_leaf(reg, state.tuple_leaf);
		else if(state.tuple_queue != NULL)
			state.set_tuple_queue(reg, state.tuple_queue);
      assert(!(state.tuple_leaf != NULL && state.tuple_queue != NULL));
		state.set_tuple(reg, state.tuple);
   } else if(val_is_stack(from)) {
      const offset_num off(pcounter_offset_num(m));
      state.set_reg(reg, *state.get_stack_at(off));
   } else if(val_is_ptr(from)) {
      const ptr_val val(pcounter_ptr(m));
      state.set_ptr(val_reg(from), val);
   } else {
      throw vm_exec_error("invalid move to reg");
   }
}

static inline void
move_to_field(pcounter m, state& state, const instr_val& from)
{
   if(val_is_bool(from)) {
      const bool_val b(pcounter_bool(m));

      pcounter_move_bool(&m);

      tuple *tuple(state.get_tuple(val_field_reg(m)));

      tuple->set_bool(val_field_num(m), b);
   } else if(val_is_float(from)) {
      const float_val flt(pcounter_float(m));
      
      pcounter_move_float(&m);
      
      tuple* tuple(state.get_tuple(val_field_reg(m)));
      
      tuple->set_float(val_field_num(m), flt);
   } else if(val_is_int(from)) {
      const int_val i(pcounter_int(m));
      
      pcounter_move_int(&m);
      
      tuple *tuple(state.get_tuple(val_field_reg(m)));
      
      tuple->set_int(val_field_num(m), i);
   } else if(val_is_node(from)) {
      const node_val val(get_node_val(m, state));
      
      tuple *tuple(state.get_tuple(val_field_reg(m)));
      
      tuple->set_node(val_field_num(m), val);
	} else if(val_is_string(from)) {
		const uint_val id(pcounter_uint(m));
		
		pcounter_move_uint(&m);
		
		tuple *tuple(state.get_tuple(val_field_reg(m)));
		
		tuple->set_string(val_field_num(m), state.all->PROGRAM->get_default_string(id));
	} else if(val_is_arg(from)) {
		const argument_id id(pcounter_argument_id(m));
		
		pcounter_move_argument_id(&m);
		
		tuple *tuple(state.get_tuple(val_field_reg(m)));
		
		tuple->set_string(val_field_num(m), state.all->get_argument(id));
   } else if(val_is_stack(from)) {
      const offset_num off(pcounter_offset_num(m));

      pcounter_move_offset_num(&m);

		tuple *tuple(state.get_tuple(val_field_reg(m)));

		const field_num to_field(val_field_num(m));

      tuple->set_field(to_field, *state.get_stack_at(off));
	} else if(val_is_const(from)) {
		const const_id cid(pcounter_const_id(m));
		
		pcounter_move_const_id(&m);
		
		tuple *tuple(state.get_tuple(val_field_reg(m)));
		
		const field_num to_field(val_field_num(m));
		const field_type typ(tuple->get_field_type(to_field)->get_type());
		
		switch(typ) {
			case FIELD_INT:	
				tuple->set_int(to_field, state.all->get_const_int(cid));
				break;
			case FIELD_FLOAT:
				tuple->set_float(to_field, state.all->get_const_float(cid));
				break;
         case FIELD_LIST:
            tuple->set_cons(to_field, state.all->get_const_cons(cid));
				break;
         case FIELD_NODE:
            tuple->set_node(to_field, state.all->get_const_node(cid));
            break;
			default: throw vm_exec_error("don't know how to move to " + field_type_string(typ) + " field (move_to_field from const)");
		}
   } else if(val_is_field(from)) {
      const tuple *from_tuple(state.get_tuple(val_field_reg(m)));
      const field_num from_field(val_field_num(m));
      
      pcounter_move_field(&m);
      
      tuple *to_tuple(state.get_tuple(val_field_reg(m)));
      const field_num to_field(val_field_num(m));
      
      switch(to_tuple->get_field_type(to_field)->get_type()) {
         case FIELD_INT:
            to_tuple->set_int(to_field, from_tuple->get_int(from_field));
            break;
         case FIELD_FLOAT:
            to_tuple->set_float(to_field, from_tuple->get_float(from_field));
            break;
         case FIELD_LIST:
            to_tuple->set_cons(to_field, from_tuple->get_cons(from_field));
            break;
         case FIELD_NODE:
            to_tuple->set_node(to_field, from_tuple->get_node(from_field));
            break;
         case FIELD_STRING:
            to_tuple->set_string(to_field, from_tuple->get_string(from_field));
            break;
         default:
            throw vm_exec_error("don't know how to move to field (move_to_field)");
      }
   } else {
      tuple* tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      
      if(val_is_host(from))
         tuple->set_node(field, state.node->get_id());
      else if(val_is_reg(from)) {
         const reg_num reg(val_reg(from));
         
         switch(tuple->get_field_type(field)->get_type()) {
            case FIELD_INT:
               tuple->set_int(field, state.get_int(reg));
               break;
            case FIELD_FLOAT:
               tuple->set_float(field, state.get_float(reg));
               break;
            case FIELD_NODE:
               tuple->set_node(field, state.get_node(reg));
               break;
            case FIELD_LIST:
               tuple->set_cons(field, state.get_cons(reg));
               break;
				case FIELD_STRING:
					tuple->set_string(field, state.get_string(reg));
					break;
            default: throw vm_exec_error("do not know how to move reg to this tuple field");
         }
      } else
         throw vm_exec_error("invalid move to field (move_to_field)");
   }
}

static inline void
move_to_stack(pcounter pc, pcounter m, state& state, const instr_val& from)
{
   if(val_is_pcounter(from)) {
      const int off((int)pcounter_offset_num(m));
      SET_FIELD_PTR(*(state.get_stack_at(off)), advance(pc));
   } else if(val_is_int(from)) {
		const int_val it(pcounter_int(m));
		
		pcounter_move_int(&m);

      const offset_num off(pcounter_offset_num(m));
      SET_FIELD_INT(*(state.get_stack_at(off)), it);
   } else if(val_is_node(from)) {
      const node_val n(pcounter_node(m));

      pcounter_move_node(&m);

      const offset_num off(pcounter_offset_num(m));
      SET_FIELD_FLOAT(*(state.get_stack_at(off)), n);
   } else if(val_is_float(from)) {
		const float_val flt(pcounter_float(m));
      
      pcounter_move_float(&m);

      const offset_num off(pcounter_offset_num(m));
      SET_FIELD_FLOAT(*(state.get_stack_at(off)), flt);

   } else if(val_is_reg(from)) {
		const reg_num reg(val_reg(from));
      const offset_num off(pcounter_offset_num(m));
      *(state.get_stack_at(off)) = state.get_reg(reg);
	} else if(val_is_const(from)) {
		const const_id cid(pcounter_const_id(m));
		
		pcounter_move_const_id(&m);

      const offset_num off(pcounter_offset_num(m));
      *(state.get_stack_at(off)) = state.all->get_const(cid);
   } else
		throw vm_exec_error("invalid move to stack (move_to_stack)");
}

static inline void
move_to_const(pcounter m, state& state, const instr_val& from)
{
	if(val_is_float(from)) {
		const float_val flt(pcounter_float(m));
      
      pcounter_move_float(&m);
      
		const const_id cid(pcounter_const_id(m));

		state.all->set_const_float(cid, flt);
	} else if(val_is_int(from)) {
		const int_val it(pcounter_int(m));
		
		pcounter_move_int(&m);
		
		const const_id cid(pcounter_int(m));
		
		state.all->set_const_int(cid, it);
	} else if(val_is_reg(from)) {
		const reg_num reg(val_reg(from));
		const const_id cid(pcounter_const_id(m));
		
		state.copy_reg2const(reg, cid);
	} else if(val_is_node(from)) {
		const node_val node(get_node_val(m, state));
		
		const const_id cid(pcounter_const_id(m));
		state.all->set_const_node(cid, node);
   } else if(val_is_string(from)) {
		const uint_val id(pcounter_uint(m));
		
		pcounter_move_uint(&m);

      rstring::ptr str(state.all->PROGRAM->get_default_string(id));

		const const_id cid(pcounter_const_id(m));

      state.all->set_const_string(cid, str);
	} else
		throw vm_exec_error("invalid move to const (move_to_const)");
}

static inline void
move_to_pcounter(pcounter& pc, const pcounter pm, state& state, const instr_val from)
{
   if(val_is_stack(from)) {
      const offset_num off(pcounter_offset_num(pm));
      pc = (pcounter)FIELD_PCOUNTER(*(state.get_stack_at(off)));
   } else if(val_is_ptr(from)) {
      const ptr_val val(pcounter_ptr(pm));
      pc = (pcounter)val;
   } else
      throw vm_exec_error("invalid move to pcounter (move_to_pcounter)");
}

static inline void
execute_move(pcounter& pc, state& state)
{
   const instr_val to(move_to(pc));
   
   if(val_is_reg(to))
      move_to_reg(pc + MOVE_BASE, state, val_reg(to), move_from(pc));
   else if(val_is_field(to))
      move_to_field(pc + MOVE_BASE, state, move_from(pc));
	else if(val_is_const(to))
		move_to_const(pc + MOVE_BASE, state, move_from(pc));
   else if(val_is_stack(to))
      move_to_stack(pc, pc + MOVE_BASE, state, move_from(pc));
   else if(val_is_pcounter(to))
      move_to_pcounter(pc, pc + MOVE_BASE, state, move_from(pc));
   else
      throw vm_exec_error("invalid move target");
}

static inline void
execute_alloc(const pcounter& pc, state& state)
{
   tuple *tuple(state.all->PROGRAM->new_tuple(alloc_predicate(pc)));

   state.set_tuple(alloc_reg(pc), tuple);
}

static inline void
execute_send_self(tuple *tuple, state& state)
{
#ifdef DEBUG_SENDS
   print_mtx.lock();
   ostringstream ss;
   ss << "\t" << *tuple << " " << state.count << " -> self " << state.node->get_id() << " (" << state.depth << ")" << endl;
   cout << ss.str();
   print_mtx.unlock();
#endif
   if(tuple->is_action()) {
      if(state.count > 0)
         state.all->MACHINE->run_action(state.sched,
               state.node, tuple);
      else
         delete tuple;
      return;
   }

   if(state.use_local_tuples || state.persistent_only) {
      const predicate *pred(tuple->get_predicate());

#ifdef USE_UI
      if(state::UI) {
         if(tuple->is_persistent()) {
            LOG_PERSISTENT_DERIVATION(state.node, tuple);
         } else if(tuple->is_linear() && !tuple->is_action()) {
            LOG_LINEAR_DERIVATION(state.node, tuple);
         }
      }
#endif
      if(pred->get_strat_level() != state.current_level) {
         simple_tuple *stuple(new simple_tuple(tuple, state.count, state.depth));
         assert(stuple->can_be_consumed());
         state.generated_other_level.push_back(stuple);
      } else {
         if(tuple->is_persistent()) {
            simple_tuple *stuple(new simple_tuple(tuple, state.count, state.depth));
            state.generated_persistent_tuples.push_back(stuple);
         } else {
            simple_tuple *stuple(new simple_tuple(tuple, state.count, state.depth));
            if(tuple->is_reused()) // push into persistent list, since it is a reused tuple
               state.generated_persistent_tuples.push_back(stuple);
            else
               state.generated_tuples.push_back(stuple);

            state.node->matcher.register_tuple(tuple, 1);
            state.mark_predicate_to_run(pred);
         }
      }
   } else {
      simple_tuple *stuple(new simple_tuple(tuple, state.count, state.depth));
      state.all->MACHINE->route_self(state.sched, state.node, stuple);
      state.add_generated_tuple(stuple);
   }
}

static inline void
execute_send(const pcounter& pc, state& state)
{
   const reg_num msg(send_msg(pc));
   const reg_num dest(send_dest(pc));
   const node_val dest_val(state.get_node(dest));
   tuple *tuple(state.get_tuple(msg));

   if(state.count < 0 && tuple->is_linear() && !tuple->is_reused()) {
      delete tuple;
      return;
   }

#ifdef CORE_STATISTICS
   state.stat.stat_predicate_proven[tuple->get_predicate_id()]++;
#endif

   if(msg == dest) {
      execute_send_self(tuple, state);
   } else {
#ifdef DEBUG_SENDS
      print_mtx.lock();
      ostringstream ss;
      ss << "\t" << *tuple << " " << state.count << " -> " << dest_val << " (" << state.depth << ")" << endl;
      cout << ss.str();
      print_mtx.unlock();
#endif
#ifdef USE_UI
      if(state::UI) {
         LOG_TUPLE_SEND(state.node, state.all->DATABASE->find_node((node::node_id)dest_val), tuple);
      }
#endif
      simple_tuple *stuple(new simple_tuple(tuple, state.count, state.depth));
      state.all->MACHINE->route(state.node, state.sched, (node::node_id)dest_val, stuple);
   }
}

static inline void
execute_send_delay(const pcounter& pc, state& state)
{
   const reg_num msg(send_delay_msg(pc));
   const reg_num dest(send_delay_dest(pc));
   const node_val dest_val(state.get_node(dest));
   tuple *tuple(state.get_tuple(msg));

#ifdef CORE_STATISTICS
   state.stat.stat_predicate_proven[tuple->get_predicate_id()]++;
#endif

   simple_tuple *stuple(new simple_tuple(tuple, state.count, state.depth));

   if(msg == dest) {
#ifdef DEBUG_SENDS
      cout << "\t" << *tuple << " -> self " << state.node->get_id() << endl;
#endif
      state.all->MACHINE->route_self(state.sched, state.node, stuple, send_delay_time(pc));
   } else {
#ifdef DEBUG_SENDS
      cout << "\t" << *tuple << " -> " << dest_val << endl;
#endif
#ifdef USE_UI
      if(state::UI) {
         LOG_TUPLE_SEND(state.node, state.all->DATABASE->find_node((node::node_id)dest_val), tuple);
      }
#endif
      state.all->MACHINE->route(state.node, state.sched, (node::node_id)dest_val, stuple, send_delay_time(pc));
   }
}

template <typename T>
static inline T get_op_function(const instr_val& val, pcounter& m, state& state);

template <>
bool_val get_op_function<bool_val>(const instr_val& val, pcounter& m, state& state)
{
   if(val_is_reg(val)) {
      return state.get_bool(val_reg(val));
   } else if(val_is_field(val)) {
      const tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));

      pcounter_move_field(&m);
      return tuple->get_bool(field);
   } else
      throw vm_exec_error("invalid bool for bool op");
}

template <>
float_val get_op_function<float_val>(const instr_val& val, pcounter& m, state& state)
{
   if(val_is_float(val)) {
      const float_val flt(pcounter_float(m));
      pcounter_move_float(&m);
      return flt;
   } else if(val_is_reg(val)) {
      return state.get_float(val_reg(val));
   } else if(val_is_field(val)) {
      const tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      pcounter_move_field(&m);
      
      return tuple->get_float(field);
	} else if(val_is_const(val)) {
		const const_id cid(pcounter_const_id(m));
		pcounter_move_const_id(&m);
		float_val val(state.all->get_const_float(cid));
		
		return val;
   } else
      throw vm_exec_error("invalid float for float op");
}

template <>
tuple_field get_op_function<tuple_field>(const instr_val& val, pcounter& m, state& state)
{
   tuple_field ret;
   if(val_is_int(val)) {
      SET_FIELD_INT(ret, pcounter_int(m));
      pcounter_move_int(&m);
   } else if(val_is_host(val))
      SET_FIELD_NODE(ret, state.node->get_id());
   else if(val_is_float(val)) {
      SET_FIELD_FLOAT(ret, pcounter_float(m));
      pcounter_move_float(&m);
   } else if(val_is_reg(val))
      ret = state.get_reg(val_reg(val));
   else if(val_is_field(val)) {
      const tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      
      pcounter_move_field(&m);
      
      ret = tuple->get_field(field);
   } else {
      throw vm_exec_error("invalid data (get_op_function<tuple_field>)");
   }

   return ret;
}

template <>
int_val get_op_function<int_val>(const instr_val& val, pcounter& m, state& state)
{
   if(val_is_int(val)) {
      const int_val i(pcounter_int(m));
      pcounter_move_int(&m);
      return i;
   } else if(val_is_reg(val))
      return state.get_int(val_reg(val));
   else if(val_is_field(val)) {
      const tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      
      pcounter_move_field(&m);
      
      return tuple->get_int(field);
	} else if(val_is_const(val)) {
		const const_id cid(pcounter_const_id(m));
		
		pcounter_move_const_id(&m);
		
		return state.all->get_const_int(cid);
   } else
      throw vm_exec_error("invalid int for int op");
}

template <>
node_val get_op_function<node_val>(const instr_val& val, pcounter& m, state& state)
{
   if(val_is_host(val))
      return state.node->get_id();
   else if(val_is_node(val))
      return get_node_val(m, state);
   else if(val_is_reg(val))
      return state.get_node(val_reg(val));
   else if(val_is_field(val)) {
      const tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      
      pcounter_move_field(&m);
      
      return tuple->get_node(field);
	} else if(val_is_const(val)) {
		const const_id cid(pcounter_const_id(m));
		pcounter_move_const_id(&m);
		
		return state.all->get_const_node(cid);
   } else
      throw vm_exec_error("invalid node for node op (get_op_function<node_val>)");
}

template <>
ptr_val get_op_function<ptr_val>(const instr_val& val, pcounter& m, state& state)
{
   if(val_is_reg(val))
      return state.get_ptr(val_reg(val));
   else if(val_is_field(val)) {
      const tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      
      pcounter_move_field(&m);
      
      return tuple->get_ptr(field);
   } else
      throw vm_exec_error("invalid ptr for ptr op");
}

template <>
cons* get_op_function<cons*>(const instr_val& val, pcounter& m, state& state)
{
   if(val_is_reg(val))
      return state.get_cons(val_reg(val));
   else if(val_is_field(val)) {
      const tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      
      pcounter_move_field(&m);
      
      return tuple->get_cons(field);
   } else if(val_is_nil(val))
      return cons::null_list();
	else if(val_is_const(val)) {
		const const_id cid(pcounter_const_id(m));
		
		pcounter_move_const_id(&m);
		
		return state.all->get_const_cons(cid);
	} else
      throw vm_exec_error("unable to get a cons (get_op_function<cons*>)");
}

template <>
struct1* get_op_function<struct1*>(const instr_val& val, pcounter& m, state& state)
{
   if(val_is_reg(val))
      return state.get_struct(val_reg(val));
   else if(val_is_field(val)) {
      const tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      
      pcounter_move_field(&m);
      
      return tuple->get_struct(field);
   } else if(val_is_const(val)) {
		const const_id cid(pcounter_const_id(m));
		
		pcounter_move_const_id(&m);
		
		return state.all->get_const_struct(cid);
	} else
      throw vm_exec_error("unable to get a struct (get_op_function<struct1*>)");
}

template <>
rstring::ptr get_op_function<rstring::ptr>(const instr_val& val, pcounter& m, state& state)
{
	if(val_is_reg(val))
		return state.get_string(val_reg(val));
	else if(val_is_field(val)) {
		const tuple *tuple(state.get_tuple(val_field_reg(m)));
		const field_num field(val_field_num(m));
		
		pcounter_move_field(&m);
		
		return tuple->get_string(field);
	} else if(val_is_string(val)) {
		const uint_val id(pcounter_uint(m));
		
		pcounter_move_uint(&m);
		
		return state.all->PROGRAM->get_default_string(id);
	} else if(val_is_arg(val)) {
		const argument_id id(pcounter_argument_id(m));
		
		pcounter_move_argument_id(&m);

		rstring::ptr s(state.all->get_argument(id));
		state.add_string(s);
		
		return s;
   } else if(val_is_const(val)) {
		const const_id cid(pcounter_const_id(m));
		
		pcounter_move_const_id(&m);
		
		return state.all->get_const_string(cid);
	} else
		throw vm_exec_error("unable to get a string");
}

template <typename T>
static inline void set_op_function(const pcounter& m,
   const instr_val& dest, T val, state& state);

template <>
void set_op_function<struct1*>(const pcounter& m, const instr_val& dest,
      struct1 *s, state& state)
{
   if(val_is_reg(dest))
      state.set_struct(val_reg(dest), s);
   else if(val_is_field(dest)) {
      tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      
      tuple->set_struct(field, s);
   } else
      throw vm_exec_error("invalid destination for a struct value");
}

template <>
void set_op_function<rstring*>(const pcounter& m, const instr_val& dest,
      rstring *s, state& state)
{
   if(val_is_reg(dest))
      state.set_string(val_reg(dest), s);
   else if(val_is_field(dest)) {
      tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      
      tuple->set_string(field, s);
   } else
      throw vm_exec_error("invalid destination for a string value");
}

template <>
void set_op_function<bool_val>(const pcounter& m, const instr_val& dest,
   bool_val val, state& state)
{
   (void)m;
   
   if(val_is_reg(dest))
      state.set_bool(val_reg(dest), val);
   else if(val_is_field(dest))
      throw vm_exec_error("can't put a bool in a tuple field - yet");
   else
      throw vm_exec_error("invalid destination for bool value");
}

template <>
void set_op_function<int_val>(const pcounter& m, const instr_val& dest,
   int_val val, state& state)
{
   if(val_is_reg(dest))
      state.set_int(val_reg(dest), val);
   else if(val_is_field(dest)) {
      tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      
      tuple->set_int(field, val);
   } else if(val_is_stack(dest)) {
      const offset_num off(pcounter_offset_num(m));
      SET_FIELD_INT(*(state.get_stack_at(off)), val);
   } else
      throw vm_exec_error("invalid destination for int value");
}

template <>
void set_op_function<float_val>(const pcounter& m, const instr_val& dest,
   float_val val, state& state)
{
   if(val_is_reg(dest))
      state.set_float(val_reg(dest), val);
   else if(val_is_field(dest)) {
      tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      
      tuple->set_float(field, val);
   } else if(val_is_stack(dest)) {
      const offset_num off(pcounter_offset_num(m));

      SET_FIELD_FLOAT(*(state.get_stack_at(off)), val);
   } else
      throw vm_exec_error("invalid destination for float value");
}

template <>
void set_op_function<node_val>(const pcounter& m, const instr_val& dest,
   node_val val, state& state)
{
   if(val_is_reg(dest))
      state.set_node(val_reg(dest), val);
   else if(val_is_field(dest)) {
      tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      
      tuple->set_node(field, val);
   } else
      throw vm_exec_error("invalid destination for addr value");
}

template <>
void set_op_function<cons*>(const pcounter& m, const instr_val& dest,
   cons* val, state& state)
{
   if(val_is_reg(dest))
      state.set_cons(val_reg(dest), val);
   else if(val_is_field(dest)) {
      tuple *tuple(state.get_tuple(val_field_reg(m)));
      const field_num field(val_field_num(m));
      
      tuple->set_cons(field, val);
   } else
      throw vm_exec_error("invalid destination for cons value");
}

static inline void
execute_op(const pcounter& pc, state& state)
{
   const instr_val arg1(op_arg1(pc));
   const instr_val arg2(op_arg2(pc));
   const instr_val dest(op_dest(pc));
   const instr_op op(op_op(pc));
   pcounter m = pc + OP_BASE;
   
#define implement_operation(TYPE_ARGS, TYPE_RET, OP)               { \
   const TYPE_ARGS v1(get_op_function<TYPE_ARGS>(arg1, m, state));   \
   const TYPE_ARGS v2(get_op_function<TYPE_ARGS>(arg2, m, state));   \
   set_op_function<TYPE_RET>(m, dest, (TYPE_RET)(OP), state);        \
   break;                                                            \
}

   switch(op) {
      case OP_NEQF: implement_operation(float_val, bool_val, v1 != v2);
      case OP_NEQI: implement_operation(int_val, bool_val, v1 != v2);
      case OP_EQF: implement_operation(float_val, bool_val, v1 == v2);
      case OP_EQI: implement_operation(int_val, bool_val, v1 == v2);
      case OP_LESSF: implement_operation(float_val, bool_val, v1 < v2);
      case OP_LESSI: implement_operation(int_val, bool_val, v1 < v2);
      case OP_LESSEQF: implement_operation(float_val, bool_val, v1 <= v2);
      case OP_LESSEQI: implement_operation(int_val, bool_val, v1 <= v2);
      case OP_GREATERF: implement_operation(float_val, bool_val, v1 > v2);
      case OP_GREATERI: implement_operation(int_val, bool_val, v1 > v2);
      case OP_GREATEREQF: implement_operation(float_val, bool_val, v1 >= v2);
      case OP_GREATEREQI: implement_operation(int_val, bool_val, v1 >= v2);
      case OP_MODF: implement_operation(float_val, float_val, fmod(v1,v2));
      case OP_MODI: implement_operation(int_val, int_val, v1 % v2);
      case OP_PLUSF: implement_operation(float_val, float_val, v1 + v2);
      case OP_PLUSI: implement_operation(int_val, int_val, v1 + v2);
      case OP_MINUSF: implement_operation(float_val, float_val, v1 - v2);
      case OP_MINUSI: implement_operation(int_val, int_val, v1 - v2);
      case OP_TIMESF: implement_operation(float_val, float_val, v1 * v2);
      case OP_TIMESI: implement_operation(int_val, int_val, v1 * v2);
      case OP_DIVF: implement_operation(float_val, float_val, v1 / v2);
      case OP_DIVI: implement_operation(int_val, int_val, v1 / v2);
      case OP_NEQA: implement_operation(node_val, bool_val, v1 != v2);
      case OP_EQA: implement_operation(node_val, bool_val, v1 == v2);
      case OP_GREATERA: implement_operation(node_val, bool_val, v1 > v2);
      case OP_ORB: implement_operation(bool_val, bool_val, v1 || v2);
      default: throw vm_exec_error("unknown operation (execute_op)");
   }
#undef implement_operation
}

static inline void
execute_not(pcounter& pc, state& state)
{
   const instr_val op(not_op(pc));
   const instr_val dest(not_dest(pc));
   pcounter m = pc + NOT_BASE;
   bool_val val;
   
   if(val_is_reg(op))
      val = state.get_bool(val_reg(op));
   else if(val_is_field(op)) {   
      const tuple *tuple(state.get_tuple(val_field_reg(m)));
      val = tuple->get_bool(val_field_num(m));
      
      pcounter_move_field(&m);
   } else
      throw vm_exec_error("invalid source for not instruction");
   
   // invert value
   val = !val;
   
   if(val_is_reg(dest))
      state.set_bool(val_reg(dest), val);
   else if(val_is_field(dest)) {
      tuple *tuple(state.get_tuple(val_field_reg(m)));
      
      tuple->set_bool(val_field_num(m), val);
   } else
      throw vm_exec_error("invalid destination for not instruction");
}

static inline bool
do_match(const tuple *tuple, const field_num& field, const instr_val& val,
   pcounter &pc, const state& state)
{
   if(val_is_reg(val)) {
      const reg_num reg(val_reg(val));
      
      switch(tuple->get_field_type(field)->get_type()) {
         case FIELD_INT: return tuple->get_int(field) == state.get_int(reg);
         case FIELD_FLOAT: return tuple->get_float(field) == state.get_float(reg);
         case FIELD_NODE: return tuple->get_node(field) == state.get_node(reg);
         default: throw vm_exec_error("matching with non-primitive types in registers is unsupported");
      }
   } else if(val_is_field(val)) {
      const vm::tuple *tuple2(state.get_tuple(val_field_reg(pc)));
      const field_num field2(val_field_num(pc));
      
      pcounter_move_field(&pc);
      
      switch(tuple->get_field_type(field)->get_type()) {
         case FIELD_INT: return tuple->get_int(field) == tuple2->get_int(field2);
         case FIELD_FLOAT: return tuple->get_float(field) == tuple2->get_float(field2);
         case FIELD_NODE: return tuple->get_node(field) == tuple2->get_node(field2);
         default: throw vm_exec_error("matching with non-primitive types in fields is unsupported");
      }
   } else if(val_is_nil(val))
      throw vm_exec_error("match for NIL not implemented");
   else if(val_is_host(val))
      return tuple->get_node(field) == state.node->get_id();
   else if(val_is_int(val)) {
      const int_val i(pcounter_int(pc));
      
      pcounter_move_int(&pc);
      
      return tuple->get_int(field) == i;
   } else if(val_is_float(val)) {
      const float_val flt(pcounter_float(pc));
      
      pcounter_move_float(&pc);
      
      return tuple->get_float(field) == flt;
   } else
      throw vm_exec_error("match value in iter is not valid");
}

static inline bool
do_matches(pcounter pc, const tuple *tuple, const state& state)
{
   if(iter_match_none(pc))
      return true;
   
   iter_match match;
   
   do {
      match = pc;
      const field_num field(iter_match_field(match));
      const instr_val val(iter_match_val(match));
      
      pcounter_move_match(&pc);
      
      if(!do_match(tuple, field, val, pc, state))
         return false;
   } while(!iter_match_end(match));
      
   return true;
}

static inline void
build_match_object(match& m, pcounter pc, state& state, const predicate *pred)
{
   if(iter_match_none(pc))
      return;
      
   iter_match match;
   
   do {
      match = pc;
      
      const field_num field(iter_match_field(match));
      const instr_val val(iter_match_val(match));
      
      pcounter_move_match(&pc);
      
      switch(pred->get_field_type(field)->get_type()) {
         case FIELD_INT: {
            const int_val i(get_op_function<int_val>(val, pc, state));
            m.match_int(field, i);
         }
         break;
         case FIELD_FLOAT: {
            const float_val f(get_op_function<float_val>(val, pc, state));
            m.match_float(field, f);
         }
         break;
         case FIELD_NODE: {
            const node_val n(get_op_function<node_val>(val, pc, state));
            m.match_node(field, n);
         }
         break;
         default: throw vm_exec_error("invalid field type for ITERATE");
      }
   } while(!iter_match_end(match));
}

typedef enum {
	ITER_DB,
	ITER_LOCAL
} iter_type_t;

typedef pair<iter_type_t, void*> iter_object;

class tuple_sorter
{
private:
	const predicate *pred;
	const field_num field;
	
	static inline tuple *get_tuple(const iter_object& l)
	{
		switch(l.first) {
			case ITER_LOCAL:
				return ((simple_tuple*)l.second)->get_tuple();
			case ITER_DB:
				return ((tuple_trie_leaf*)l.second)->get_underlying_tuple();
			default: assert(false); return NULL;
		}
	}
	
public:
	
	inline bool operator()(const iter_object& l1, const iter_object& l2)
	{
		tuple *t1(get_tuple(l1));
		tuple *t2(get_tuple(l2));
			
		assert(t1 != NULL && t2 != NULL);
		
		switch(pred->get_field_type(field)->get_type()) {
			case FIELD_INT:
				return t1->get_int(field) < t2->get_int(field);
			case FIELD_FLOAT:
				return t1->get_float(field) < t2->get_float(field);
			case FIELD_NODE:
				return t1->get_node(field) < t2->get_node(field);
			default:
				throw vm_exec_error("don't know how to compare this field type (tuple_sorter)");
		}
		
		assert(false);
		return true;
	}
	
	explicit tuple_sorter(const field_num _field, const predicate *_pred):
		pred(_pred), field(_field)
	{}
};

static inline return_type
execute_iter(pcounter pc, const utils::byte options, const utils::byte options_arguments,
		pcounter first, state& state, tuple_trie::tuple_search_iterator tuples_it, const predicate *pred)
{
   const bool old_is_linear = state.is_linear;
	const bool this_is_linear = (pred->is_linear_pred() && !state.persistent_only && iter_options_to_delete(options));

#ifndef NDEBUG
   if(pred->is_linear_pred() && state.persistent_only) {
      assert(pred->is_reused_pred());
   }
#endif
	
#define PUSH_CURRENT_STATE(TUPLE, TUPLE_LEAF, TUPLE_QUEUE, NEW_DEPTH)		\
	tuple *old_tuple = state.tuple;										            \
   tuple_trie_leaf *old_tuple_leaf = state.tuple_leaf;		            	\
	simple_tuple *old_tuple_queue = state.tuple_queue;				            \
   const depth_t old_depth = state.depth;                                  \
																					            \
   state.tuple = TUPLE;													            	\
   state.tuple_leaf = TUPLE_LEAF;										            \
	state.tuple_queue = TUPLE_QUEUE;										            \
	state.is_linear = this_is_linear || state.is_linear;                    \
   state.depth = !pred->is_cycle_pred() ? state.depth : max((NEW_DEPTH)+1, state.depth)
	
#define POP_STATE()								\
	state.tuple = old_tuple;					\
   state.tuple_leaf = old_tuple_leaf;		\
	state.tuple_queue = old_tuple_queue;	\
   state.is_linear = old_is_linear;       \
   state.depth = old_depth

   if(state.persistent_only) {
      // do nothing
   } else if(iter_options_min(options) || iter_options_random(options)) {
		typedef vector<iter_object, mem::allocator<iter_object> > vector_of_everything;
      vector_of_everything everything;
		
		if(state.use_local_tuples) {
#ifdef CORE_STATISTICS
         execution_time::scope s(state.stat.ts_search_time_predicate[pred->get_id()]);
#endif

			for(db::simple_tuple_list::iterator it(state.local_tuples.begin()), end(state.local_tuples.end());
				it != end;
				++it)
			{
				simple_tuple *stpl(*it);
				if(stpl->get_predicate() == pred && stpl->can_be_consumed()) {
					if(do_matches(pc, stpl->get_tuple(), state))
						everything.push_back(iter_object(ITER_LOCAL, (void*)stpl));
				}
			}
		}
		
		for(tuple_trie::tuple_search_iterator end(tuple_trie::match_end());
			tuples_it != end; ++tuples_it)
		{
			tuple_trie_leaf *tuple_leaf(*tuples_it);
#ifdef TRIE_MATCHING_ASSERT
	      assert(do_matches(pc, tuple_leaf->get_underlying_tuple(), state));
#endif
			everything.push_back(iter_object(ITER_DB, (void*)tuple_leaf));
		}
		
		if(iter_options_random(options))
			utils::shuffle_vector(everything, state.randgen);
		else if(iter_options_min(options)) {
			const field_num field(iter_options_min_arg(options_arguments));
			
			sort(everything.begin(), everything.end(), tuple_sorter(field, pred));
		} else throw vm_exec_error("do not know how to use this selector");

		for(vector_of_everything::iterator it(everything.begin()), end(everything.end());
			it != end; ++it)
		{
			iter_object p(*it);
			return_type ret;
			
			switch(p.first) {
				case ITER_DB: {
					tuple_trie_leaf *tuple_leaf((tuple_trie_leaf*)p.second);

			      if(this_is_linear) {
			         if(!state.linear_tuple_can_be_used(tuple_leaf))
			            continue;
			         state.using_new_linear_tuple(tuple_leaf);
			      }

			      tuple *match_tuple(tuple_leaf->get_underlying_tuple());
               assert(match_tuple != NULL);

					PUSH_CURRENT_STATE(match_tuple, tuple_leaf, NULL, tuple_leaf->get_min_depth());

			      ret = execute(first, state);

					POP_STATE();

			      if(ret != RETURN_LINEAR && ret != RETURN_DERIVED && this_is_linear)
			         state.no_longer_using_linear_tuple(tuple_leaf); // not used during derivation
				}
				break;
				case ITER_LOCAL: {
					simple_tuple *stpl((simple_tuple*)p.second);
					tuple *match_tuple(stpl->get_tuple());
					
					if(!stpl->can_be_consumed())
						continue;
					
					PUSH_CURRENT_STATE(match_tuple, NULL, stpl, stpl->get_depth());
					
					if(iter_options_to_delete(options)) {
						stpl->will_delete();
					}
					
					ret = execute(first, state);
					
					POP_STATE();
					
					if(!(ret == RETURN_LINEAR || ret == RETURN_DERIVED)) {
						stpl->will_not_delete();
					}
				}
				break;
				default: ret = RETURN_NO_RETURN; assert(false);
			}
		
			if(ret == RETURN_LINEAR)
	         return ret;
	      if(ret == RETURN_DERIVED && state.is_linear)
	         return RETURN_DERIVED;
		}
		
		return RETURN_NO_RETURN;
	}

   for(tuple_trie::tuple_search_iterator end(tuple_trie::match_end());
      tuples_it != end;
      ++tuples_it)
   {
      tuple_trie_leaf *tuple_leaf(*tuples_it);

      if(pred->is_linear_pred()) {
         if(!state.linear_tuple_can_be_used(tuple_leaf))
            continue;
         state.using_new_linear_tuple(tuple_leaf);
      }

      // we get the tuple later since the previous leaf may have been deleted
      tuple *match_tuple(tuple_leaf->get_underlying_tuple());
      assert(match_tuple != NULL);
    
#ifdef TRIE_MATCHING_ASSERT
      assert(do_matches(pc, match_tuple, state));
#else
      (void)pc;
#endif

		PUSH_CURRENT_STATE(match_tuple, tuple_leaf, NULL, tuple_leaf->get_min_depth());

      return_type ret;
      
      ret = execute(first, state);

		POP_STATE();
      
      if(ret == RETURN_LINEAR) {
         return ret;
      }

      if(ret == RETURN_DERIVED && state.is_linear) {
         return RETURN_DERIVED;
      }

      if(pred->is_linear_pred()) {
         if(ret != RETURN_LINEAR && ret != RETURN_DERIVED) {
            state.no_longer_using_linear_tuple(tuple_leaf); // not consumed because nothing was derived
         }

         if(!iter_options_to_delete(options)) {
            state.no_longer_using_linear_tuple(tuple_leaf); // cannot be consumed because it would get generated again
         }
      }
   }

#if 0
	// tuples from current queue
	if(this_is_linear) {
      assert(!state.persistent_only);

   	db::simple_tuple_vector active_tuples = state.sched->gather_active_tuples(state.node, pred->get_id());

		if(iter_options_random(options))
			utils::shuffle_vector(active_tuples, state.randgen);

		for(db::simple_tuple_vector::iterator it(active_tuples.begin()), end(active_tuples.end()); it != end; ++it) {
			simple_tuple *stpl(*it);
			tuple *match_tuple(stpl->get_tuple());
			
			if(!stpl->can_be_consumed())
				continue;
		
      	if(!do_matches(pc, match_tuple, state))
				continue;
		
			PUSH_CURRENT_STATE(match_tuple, NULL, stpl, stpl->get_depth());
		
			if(iter_options_to_delete(options) || this_is_linear) {
				assert(this_is_linear);
				stpl->will_delete(); // this will avoid future gathers of this tuple!
			}
		
			// execute...
			return_type ret = execute(first, state);
		
			POP_STATE();

			if(!(ret == RETURN_LINEAR || ret == RETURN_DERIVED)) { // tuple not consumed
				if(iter_options_to_delete(options) || this_is_linear) {
					stpl->will_not_delete(); // oops, revert
            }
			}

         if(!iter_options_to_delete(options) && this_is_linear) {
            stpl->will_not_delete();
         }
		
			if(ret == RETURN_LINEAR)
				return ret;
			if(state.is_linear && ret == RETURN_DERIVED)
				return ret;
		}
	}
#endif
	
	// current set of tuples
   if(!state.persistent_only) {
		/* XXXX
		if(iter_options_random(options))
			utils::shuffle_vector(state.local_tuples, state.randgen);
		*/
		
		for(db::simple_tuple_list::iterator it(state.local_tuples.begin()), end(state.local_tuples.end()); it != end; ++it) {
			simple_tuple *stpl(*it);
			tuple *match_tuple(stpl->get_tuple());

			if(!stpl->can_be_consumed()) {
				continue;
         }
				
			if(match_tuple->get_predicate() != pred)
				continue;
				
         {
#ifdef CORE_STATISTICS
				execution_time::scope s2(state.stat.ts_search_time_predicate[pred->get_id()]);
#endif
            if(!do_matches(pc, match_tuple, state))
               continue;
         }
		
			PUSH_CURRENT_STATE(match_tuple, NULL, stpl, stpl->get_depth());
		
			if(iter_options_to_delete(options) || pred->is_linear_pred()) {
				stpl->will_delete(); // this will avoid future uses of this tuple!
			}
		
			// execute...
			return_type ret = execute(first, state);
		
			POP_STATE();

         if(pred->is_linear_pred()) {
            if(!(ret == RETURN_LINEAR || ret == RETURN_DERIVED)) { // tuple not consumed
               stpl->will_not_delete(); // oops, revert
            }

            if(!iter_options_to_delete(options)) {
               // the tuple cannot be deleted
               // we have just marked it for deletion in order to lock it
               stpl->will_not_delete();
            }
         }

			if(ret == RETURN_LINEAR) { 
				return ret;
         }
			if(state.is_linear && ret == RETURN_DERIVED) {
				return ret;
         }
		}
	}
   
   return RETURN_NO_RETURN;
}

static inline void
execute_move_nil(pcounter& pc, state& state)
{
   const instr_val dest(move_nil_dest(pc));
   
   // optimized
   if(val_is_reg(dest))
      state.set_nil(val_reg(dest));
   else if(val_is_field(dest)) {
      tuple *tuple(state.get_tuple(val_field_reg(pc + MOVE_NIL_BASE)));
      
      tuple->set_nil(val_field_num(pc + MOVE_NIL_BASE));
   } else
      throw vm_exec_error("unsupported destination for move-nil");
}

static inline void
execute_cons(pcounter pc, state& state)
{
   pcounter m = pc + CONS_BASE;
   const instr_val head(cons_head(pc));
   const instr_val tail(cons_tail(pc));
   const instr_val dest(cons_dest(pc));
   list_type *ltype((list_type*)state.all->PROGRAM->get_type(cons_type(pc)));
   
   const tuple_field head_val(get_op_function<tuple_field>(head, m, state));
   cons *ls(get_op_function<cons*>(tail, m, state));
   cons *new_list(new cons(ls, head_val, ltype));
	state.add_cons(new_list);
   set_op_function(m, dest, new_list, state);
}

static inline void
execute_test_nil(pcounter pc, state& state)
{
   const instr_val op(test_nil_op(pc));
   const instr_val dest(test_nil_dest(pc));
   
   pc += TEST_NIL_BASE;
   
   const ptr_val val(get_op_function<ptr_val>(op, pc, state));
   
   set_op_function(pc, dest, val == null_ptr_val, state);
}

static inline void
execute_tail(pcounter& pc, state& state)
{
   pcounter m = pc + TAIL_BASE;
   const instr_val cons_val(tail_cons(pc));
   const instr_val dest(tail_dest(pc));
   
   cons *ls(get_op_function<cons*>(cons_val, m, state));
   cons *tail(ls->get_tail());
   set_op_function(m, dest, tail, state);
}

static inline void
move_typed_data(const pcounter& pc, type *t, const tuple_field& data,
      const instr_val to, state& state)
{
   switch(t->get_type()) {
      case FIELD_INT:
         set_op_function<int_val>(pc, to, FIELD_INT(data), state);
         break;
      case FIELD_FLOAT:
         set_op_function<float_val>(pc, to, FIELD_FLOAT(data), state);
         break;
      case FIELD_NODE:
         set_op_function<node_val>(pc, to, FIELD_NODE(data), state);
         break;
      case FIELD_STRING:
         set_op_function<rstring*>(pc, to, FIELD_STRING(data), state);
         break;
      case FIELD_LIST:
         set_op_function<cons*>(pc, to, FIELD_CONS(data), state);
         break;
      case FIELD_STRUCT:
         set_op_function<struct1*>(pc, to, FIELD_STRUCT(data), state);
         break;
      default:
         throw vm_exec_error("unrecognized type (move_typed_data)");
   }
}

static inline void
execute_head(pcounter& pc, state& state)
{
   pcounter m = pc + HEAD_BASE;
   const instr_val cons_val(head_cons(pc));
   const instr_val dest(head_dest(pc));

   cons *ls(get_op_function<cons*>(cons_val, m, state));
   const tuple_field val(ls->get_head());
   list_type *lt(ls->get_type());
   move_typed_data(pc, lt->get_subtype(), val, dest, state);
}

static inline void
execute_float(pcounter& pc, state& state)
{
   pcounter m = pc + FLOAT_BASE;
   const instr_val op(float_op(pc));
   const instr_val dest(float_dest(pc));
   const int_val val(get_op_function<int_val>(op, m, state));
   
   set_op_function(m, dest, static_cast<float_val>(val), state);
}

static inline pcounter
execute_select(pcounter pc, state& state)
{
   if(state.node->get_id() > state.all->DATABASE->static_max_id())
      return pc + select_size(pc);

   const pcounter hash_start(select_hash_start(pc));
   const code_size_t hashed(select_hash(hash_start, state.node->get_id()));
   
   if(hashed == 0) // no specific code
      return pc + select_size(pc);
   else
      return select_hash_code(hash_start, select_hash_size(pc), hashed);
}

static inline void
execute_colocated(pcounter pc, state& state)
{
   pcounter m = pc + COLOCATED_BASE;
   
   const instr_val first(colocated_first(pc));
   const instr_val second(colocated_second(pc));
   const reg_num dest(colocated_dest(pc));
 
   const node_val n1(get_op_function<node_val>(first, m, state));
   const node_val n2(get_op_function<node_val>(second, m, state));
   
   state.set_bool(dest, state.all->MACHINE->same_place(n1, n2));
}

static inline void
execute_delete(const pcounter pc, state& state)
{
   const predicate_id id(delete_predicate(pc));
   const predicate *pred(state.all->PROGRAM->get_predicate(id));
   pcounter m(pc + DELETE_BASE);
   const size_t num_args(delete_num_args(pc));
   match mobj(pred);
   
   assert(state.node != NULL);
   assert(num_args > 0);
   int_val idx;
   
   for(size_t i(0); i < num_args; ++i) {
      const field_num fil_ind(delete_index(m));
      const instr_val fil_val(delete_val(m));
      
      assert(fil_ind == i);
      
      m += index_size + val_size;
      
      switch(pred->get_field_type(fil_ind)->get_type()) {
         case FIELD_INT:
            idx = get_op_function<int_val>(fil_val, m, state);
            mobj.match_int(fil_ind, idx);
            break;
         case FIELD_FLOAT:
            mobj.match_float(fil_ind, get_op_function<float_val>(fil_val, m, state));
            break;
         case FIELD_NODE:
            mobj.match_node(fil_ind, get_op_function<node_val>(fil_val, m, state));
            break;
         default: assert(false);
      }
   }
   
   //cout << "Removing from " << pred->get_name() << " iteration " << idx << " node " << state.node->get_id() << endl;
   
   state.node->delete_by_index(pred, mobj);
}

static inline void
read_call_arg(argument& arg, const field_type type, pcounter& m, state& state)
{
   const instr_val val_type(call_val(m));
   
   m += val_size;
   
   switch(type) {
      case FIELD_INT: {
         const int_val val(get_op_function<int_val>(val_type, m, state));
         SET_FIELD_INT(arg, val);
      }
      break;
      case FIELD_FLOAT: {
         const float_val val(get_op_function<float_val>(val_type, m, state));
         SET_FIELD_FLOAT(arg, val);
      }
      break;
      case FIELD_NODE: {
         const node_val val(get_op_function<node_val>(val_type, m, state));
         SET_FIELD_NODE(arg, val);
      }
      break;
      case FIELD_LIST: {
         const cons *val(get_op_function<cons*>(val_type, m, state));
         SET_FIELD_CONS(arg, val);
      }
      break;
		case FIELD_STRING: {
			const rstring::ptr val(get_op_function<rstring::ptr>(val_type, m, state));
         SET_FIELD_STRING(arg, val);
		}
		break;
      default:
         throw vm_exec_error("can't read this external function argument (read_call_arg)");
   }
}

static inline void
execute_remove(pcounter pc, state& state)
{
   const reg_num reg(remove_source(pc));

#ifdef USE_UI
   if(state::UI) {
      LOG_LINEAR_CONSUMPTION(state.node, state.get_tuple(reg));
   }
#endif
#ifdef CORE_STATISTICS
   state.stat.stat_predicate_success[state.get_tuple(reg)->get_predicate_id()]++;
#endif

	const bool is_a_leaf(state.is_it_a_leaf(reg));
   vm::tuple *tpl(state.get_tuple(reg));

	if(state.use_local_tuples) {
#ifdef DEBUG_REMOVE
      cout << "\tdelete " << *tpl << endl;
#endif
		assert(tpl != NULL);
		
		if(is_a_leaf) {
			state.node->matcher.deregister_tuple(tpl, 1);
		}
		// the else case is done at state.cpp
	}

   assert(tpl != NULL);

   if(tpl->is_reused() && state.use_local_tuples) {
		state.generated_persistent_tuples.push_back(new simple_tuple(tpl, -1, state.depth));
		if(is_a_leaf)
			state.leaves_for_deletion.push_back(make_pair((predicate*)tpl->get_predicate(), state.get_leaf(reg)));
	} else {
		if(is_a_leaf) { // tuple was fetched from database
			//cout << "Remove " << *state.get_tuple(reg) << endl;
#ifdef CORE_STATISTICS
         execution_time::scope s(state.stat.db_deletion_time_predicate[tpl->get_predicate_id()]);
#endif
   		state.node->delete_by_leaf(tpl->get_predicate(), state.get_leaf(reg), 0);
		} else {
			// tuple was marked before, it will be deleted after this round
		}
	}
}

static inline void
execute_call(pcounter pc, state& state)
{
   pcounter m(pc + CALL_BASE);
   const external_function_id id(call_extern_id(pc));
   const size_t num_args(call_num_args(pc));
   const reg_num reg(call_dest(pc));
   external_function *f(lookup_external_function(id));
   const type* ret_type(f->get_return_type());
   argument args[num_args];
   
   for(size_t i(0); i < num_args; ++i)
      read_call_arg(args[i], f->get_arg_type(i)->get_type(), m, state);
   
   assert(num_args == f->get_num_args());
   
   argument ret;
   
   // call function
   switch(num_args) {
      case 0:
         ret = f->get_fun_ptr()();
         break;
      case 1:
         ret = ((external_function_ptr1)f->get_fun_ptr())(args[0]);
         break;
      case 2:
         ret = ((external_function_ptr2)f->get_fun_ptr())(args[0], args[1]);
         break;
      case 3:
         ret = ((external_function_ptr3)f->get_fun_ptr())(args[0], args[1], args[2]);
         break;
      default:
         throw vm_exec_error("vm does not support external functions with more than 3 arguments");
   }
   
   switch(ret_type->get_type()) {
      case FIELD_INT:
         state.set_int(reg, FIELD_INT(ret));
         break;
      case FIELD_FLOAT:
         state.set_float(reg, FIELD_FLOAT(ret));
         break;
      case FIELD_NODE:
         state.set_node(reg, FIELD_NODE(ret));
         break;
		case FIELD_STRING: {
			rstring::ptr s(FIELD_STRING(ret));
			
			state.set_string(reg, s);
			state.add_string(s);
			
			break;
		}
      case FIELD_LIST: {
         cons *l(FIELD_CONS(ret));

         state.set_cons(reg, l);
         if(!cons::is_null(l))
            state.add_cons(l);
         break;
      }
      default:
         throw vm_exec_error("invalid return type in call (execute_call)");
   }
}

static inline void
execute_rule(const pcounter& pc, state& state)
{
   const size_t rule_id(rule_get_id(pc));

   state.current_rule = rule_id;

#ifdef USE_UI
   if(state::UI) {
      vm::rule *rule(state.all->PROGRAM->get_rule(state.current_rule));
      LOG_RULE_START(state.node, rule);
   }
#endif

#ifdef CORE_STATISTICS
	if(state.stat.stat_rules_activated == 0 && state.stat.stat_inside_rule) {
		state.stat.stat_rules_failed++;
	}
	state.stat.stat_inside_rule = true;
	state.stat.stat_rules_activated = 0;
#endif
}

static inline void
execute_rule_done(const pcounter& pc, state& state)
{
   (void)pc;
   (void)state;

#ifdef USE_UI
   if(state::UI) {
      vm::rule *rule(state.all->PROGRAM->get_rule(state.current_rule));
      LOG_RULE_APPLIED(state.node, rule);
   }
#endif

#ifdef CORE_STATISTICS
	state.stat.stat_rules_ok++;
	state.stat.stat_rules_activated++;
#endif

#if 0
   const string rule_str(state.PROGRAM->get_rule_string(state.current_rule));

   cout << "Rule applied " << rule_str << endl;
#endif
}

static inline void
execute_new_node(const pcounter& pc, state& state)
{
   const reg_num reg(new_node_reg(pc));
   node *new_node(state.all->DATABASE->create_node());

   state.sched->init_node(new_node);

   state.set_node(reg, new_node->get_id());

#ifdef USE_UI
   if(state::UI) {
      LOG_NEW_NODE(new_node);
   }
#endif
}

static inline tuple_field
axiom_read_data(pcounter& pc, type *t)
{
   tuple_field f;

   switch(t->get_type()) {
      case FIELD_INT:
         SET_FIELD_INT(f, pcounter_int(pc));
         pcounter_move_int(&pc);
         break;
      case FIELD_FLOAT:
         SET_FIELD_FLOAT(f, pcounter_float(pc));
         pcounter_move_float(&pc);
         break;
      case FIELD_NODE:
         SET_FIELD_NODE(f, pcounter_node(pc));
         pcounter_move_node(&pc);
         break;
      case FIELD_LIST:
         if(*pc == 0) {
            pc++;
            SET_FIELD_CONS(f, runtime::cons::null_list());
         } else if(*pc == 1) {
            pc++;
            list_type *lt((list_type*)t);
            tuple_field head(axiom_read_data(pc, lt->get_subtype()));
            tuple_field tail(axiom_read_data(pc, t));
            runtime::cons *c(FIELD_CONS(tail));
            runtime::cons *nc(new runtime::cons(c, head, lt));
            SET_FIELD_CONS(f, nc);
         } else {
            assert(false);
         }
         break;

      default: assert(false);
   }

   return f;
}

static inline void
execute_new_axioms(pcounter pc, state& state)
{
   const pcounter end(pc + new_axioms_jump(pc));
   pc += NEW_AXIOMS_BASE;

   while(pc < end) {
      // read axions until the end!
      predicate_id pid(predicate_get(pc, 0));
      predicate *pred(state.all->PROGRAM->get_predicate(pid));
      tuple *tpl(new tuple(pred));

      pc++;

      for(size_t i(0), num_fields(pred->num_fields());
            i != num_fields;
            ++i)
      {
         tuple_field field(axiom_read_data(pc, pred->get_field_type(i)));

         switch(pred->get_field_type(i)->get_type()) {
            case FIELD_LIST:
               tpl->set_cons(i, FIELD_CONS(field));
               break;
            case FIELD_STRUCT:
               tpl->set_struct(i, FIELD_STRUCT(field));
               break;
            case FIELD_INT:
            case FIELD_FLOAT:
            case FIELD_NODE:
               tpl->set_field(i, field);
               break;
            case FIELD_STRING:
               tpl->set_string(i, FIELD_STRING(field));
               break;
            default:
               throw vm_exec_error("don't know how to handle this type (execute_new_axioms)");
         }
      }
      execute_send_self(tpl, state);
   }
}

static inline void
execute_make_struct(pcounter pc, state& state)
{
   const instr_val to(make_struct_to(pc));
   struct_type *st((struct_type*)state.all->PROGRAM->get_type(make_struct_type(pc)));

   pc += MAKE_STRUCT_BASE;
   struct1 *s(new struct1(st));

   for(size_t i(state.stack.size() - st->get_size()), j(0); i < state.stack.size(); ++i, ++j) {
      s->set_data(j, state.stack[i]);
   }
   state.stack.resize(state.stack.size() - st->get_size());

   set_op_function<struct1*>(pc, to, s, state);
   state.add_struct(s);
}

static inline void
execute_struct_val(pcounter pc, state& state)
{
   const size_t idx(struct_val_idx(pc));
   const instr_val from(struct_val_from(pc));
   const instr_val to(struct_val_to(pc));
   pcounter m = pc + STRUCT_VAL_BASE;
   struct1 *s(get_op_function<struct1*>(from, m, state));
   struct_type *st(s->get_type());
   type *t(st->get_type(idx));

   move_typed_data(pc, t, s->get_data(idx), to, state);
}

static inline return_type
execute(pcounter pc, state& state)
{
#ifdef CORE_STATISTICS
	if(state.tuple != NULL) {
		state.stat.stat_tuples_used++;
      if(state.tuple->is_linear()) {
         state.stat.stat_predicate_applications[state.tuple->get_predicate_id()]++;
      }
   }
#endif

   for(; ; pc = advance(pc))
   {
eval_loop:

#ifdef DEBUG_INSTRS
         instr_print_simple(pc, 0, state.all->PROGRAM, cout);
#endif

#ifdef USE_SIM
      if(state::SIM) {
         ++state.sim_instr_counter;
      }
#endif

#ifdef CORE_STATISTICS
		state.stat.stat_instructions_executed++;
#endif
		
      switch(fetch(pc)) {
         case RETURN_INSTR: return RETURN_OK;
         
         case NEXT_INSTR: return RETURN_NEXT;

         case RETURN_LINEAR_INSTR: return RETURN_LINEAR;
         
         case RETURN_DERIVED_INSTR: return RETURN_DERIVED;
         
         case RETURN_SELECT_INSTR:
            pc += return_select_jump(pc);
            goto eval_loop;
         
         case IF_INSTR:
#ifdef CORE_STATISTICS
				state.stat.stat_if_tests++;
#endif
            if(!state.get_bool(if_reg(pc))) {
#ifdef CORE_STATISTICS
					state.stat.stat_if_failed++;
#endif
               pc += if_jump(pc);
               goto eval_loop;
            }
            break;
         
         case ELSE_INSTR:
            throw vm_exec_error("ELSE instruction not supported");
         
			case END_LINEAR_INSTR:
				return RETURN_END_LINEAR;
			
         case RESET_LINEAR_INSTR:
            {
               const bool old_is_linear(state.is_linear);
               
               state.is_linear = false;
               
               return_type ret(execute(pc + RESET_LINEAR_BASE, state));

					assert(ret == RETURN_END_LINEAR);
               (void)ret;

               state.is_linear = old_is_linear;
               
               pc += reset_linear_jump(pc);

               goto eval_loop;
            }
            break;
            
         case ITER_INSTR: {
               const predicate_id pred_id(iter_predicate(pc));
               const predicate *pred(state.all->PROGRAM->get_predicate(pred_id));
               match mobj(pred);
               
#ifdef CORE_STATISTICS
					state.stat.stat_db_hits++;
#endif
               build_match_object(mobj, pc + ITER_BASE, state, pred);
#ifdef CORE_STATISTICS
                  //execution_time::scope s(state.stat.db_search_time_predicate[pred_id]);
#endif
               tuple_trie::tuple_search_iterator it = state.node->match_predicate(pred_id, mobj);

               const return_type ret(execute_iter(pc + ITER_BASE,
								iter_options(pc), iter_options_argument(pc),
								advance(pc), state, it, pred));
                  
               if(ret == RETURN_LINEAR)
                 return ret;
					if(state.is_linear && ret == RETURN_DERIVED)
						return ret;
               
               pc += iter_jump(pc);
               goto eval_loop;
            }
            
         case REMOVE_INSTR:
            execute_remove(pc, state);
            break;
            
         case MOVE_INSTR:
#ifdef CORE_STATISTICS
				state.stat.stat_moves_executed++;
#endif
            execute_move(pc, state);
            break;
            
         case ALLOC_INSTR:
            execute_alloc(pc, state);
            break;
            
         case SEND_INSTR:
            execute_send(pc, state);
            break;

         case SEND_DELAY_INSTR:
            execute_send_delay(pc, state);
            break;
            
         case OP_INSTR:
#ifdef CORE_STATISTICS
				state.stat.stat_ops_executed++;
#endif
            execute_op(pc, state);
            break;
            
         case NOT_INSTR:
            execute_not(pc, state);
            break;
            
         case MOVE_NIL_INSTR:
            execute_move_nil(pc, state);
            break;
            
         case CONS_INSTR:
            execute_cons(pc, state);
            break;
            
         case TEST_NIL_INSTR:
            execute_test_nil(pc, state);
            break;
            
         case TAIL_INSTR:
            execute_tail(pc, state);
            break;
            
         case HEAD_INSTR:
            execute_head(pc, state);
            break;
            
         case FLOAT_INSTR:
            execute_float(pc, state);
            break;
            
         case SELECT_INSTR:
            pc = execute_select(pc, state);
            goto eval_loop;
            
         case COLOCATED_INSTR:
            execute_colocated(pc, state);
            break;
            
         case DELETE_INSTR:
            execute_delete(pc, state);
            break;
            
         case CALL_INSTR:
            execute_call(pc, state);
            break;

         case RULE_INSTR:
           execute_rule(pc, state);
           break;

         case RULE_DONE_INSTR:
           execute_rule_done(pc, state);
           break;

         case NEW_NODE_INSTR:
           execute_new_node(pc, state);
           break;

         case NEW_AXIOMS_INSTR:
           execute_new_axioms(pc, state);
           break;

         case PUSH_INSTR:
           state.stack.push_back(tuple_field());
           break;

         case POP_INSTR:
           state.stack.pop_back();
           break;

         case PUSH_REGS_INSTR:
           state.stack.insert(state.stack.end(),
                 state.regs, state.regs + NUM_REGS);
           break;

         case POP_REGS_INSTR:
           copy(state.stack.end() - NUM_REGS, state.stack.end(), state.regs);
           state.stack.resize(state.stack.size() - NUM_REGS);
           break;

         case CALLF_INSTR: {
              const vm::callf_id id(callf_get_id(pc));
              function *fun(state.all->PROGRAM->get_function(id));

              pc = fun->get_bytecode();
              goto eval_loop;
           }
           break;

         case MAKE_STRUCT_INSTR:
           execute_make_struct(pc, state);
           break;

         case STRUCT_VAL_INSTR:
           execute_struct_val(pc, state);
           break;

         default: throw vm_exec_error("unsupported instruction");
      }
   }
}

static inline return_type
do_execute(byte_code code, state& state)
{
   assert(state.stack.empty());
   const return_type ret(execute((pcounter)code, state));

   state.cleanup();
   assert(state.stack.empty());
   return ret;
}
   
execution_return
execute_bytecode(byte_code code, state& state)
{
   const return_type ret(do_execute(code, state));
	state.unmark_generated_tuples();
	
#ifdef CORE_STATISTICS
#endif
   
   if(ret == RETURN_LINEAR) {
      return EXECUTION_CONSUMED;
	} else {
      return EXECUTION_OK;
	}
}

void
execute_rule(const rule_id rule_id, state& state)
{
#ifdef DEBUG_RULES
	cout << "Running rule " << state.all->PROGRAM->get_rule(rule_id)->get_string() << endl;
#endif
#ifdef CORE_STATISTICS
   execution_time::scope s(state.stat.rule_times[rule_id]);
#endif
   
   //   state.node->print(cout);
   //   state.all->DATABASE->print_db(cout);
	
	vm::rule *rule(state.all->PROGRAM->get_rule(rule_id));

   do_execute(rule->get_bytecode(), state);

#ifdef CORE_STATISTICS
   if(state.stat.stat_rules_activated == 0)
      state.stat.stat_rules_failed++;
#endif

}

}
