
#include <iostream>
#include <algorithm>
#include <functional>
#include <cmath>
#include <sstream>

#include "vm/exec.hpp"
#include "vm/tuple.hpp"
#include "vm/match.hpp"
#include "db/tuple.hpp"
#include "runtime/ref_base.hpp"
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

static inline return_type execute(pcounter, state&, const reg_num, tuple*);

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

static inline tuple*
get_tuple_field(state& state, const pcounter& pc)
{
   return state.get_tuple(val_field_reg(pc));
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
   if(tuple->is_persistent()) {
      simple_tuple *stuple(new simple_tuple(tuple, state.count, state.depth));
      state.generated_persistent_tuples.push_back(stuple);
   } else {
      if(tuple->is_reused()) { // push into persistent list, since it is a reused tuple
         simple_tuple *stuple(new simple_tuple(tuple, state.count, state.depth));
         state.generated_persistent_tuples.push_back(stuple);
      } else {
         simple_tuple *stuple(new simple_tuple(tuple, state.count, state.depth));
         state.generated_tuples.push_back(stuple);
      }

      state.node->matcher.register_tuple(tuple, 1);
      state.mark_predicate_to_run(pred);
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
      const offset_num off(pcounter_stack(m));
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
      const offset_num off(pcounter_stack(m));

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
execute_not(pcounter& pc, state& state)
{
   const reg_num op(not_op(pc));
   const reg_num dest(not_dest(pc));

   state.set_bool(dest, !state.get_bool(op));
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
      return runtime::cons::is_null(tuple->get_cons(field));
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
   } else if(val_is_non_nil(val))
      return !runtime::cons::is_null(tuple->get_cons(field));
   else
      throw vm_exec_error("match value in iter is not valid");
}

static bool
do_rec_match(match_field m, tuple_field x, type *t)
{
   switch(t->get_type()) {
      case FIELD_INT:
         if(FIELD_INT(m.field) != FIELD_INT(x))
            return false;
         break;
      case FIELD_FLOAT:
         if(FIELD_FLOAT(m.field) != FIELD_FLOAT(x))
            return false;
         break;
      case FIELD_NODE:
         if(FIELD_NODE(m.field) != FIELD_NODE(x))
            return false;
         break;
      case FIELD_LIST:
         if(FIELD_PTR(m.field) == 0) {
            if(!runtime::cons::is_null(FIELD_CONS(x)))
               return false;
         } else if(FIELD_PTR(m.field) == 1) {
            if(runtime::cons::is_null(FIELD_CONS(x)))
               return false;
         } else {
            runtime::cons *ls(FIELD_CONS(x));
            if(runtime::cons::is_null(ls))
               return false;
            list_match *lm(FIELD_LIST_MATCH(m.field));
            list_type *lt((list_type*)t);
            if(lm->head.exact && !do_rec_match(lm->head, ls->get_head(), lt->get_subtype()))
               return false;
            tuple_field tail;
            SET_FIELD_CONS(tail, ls->get_tail());
            if(lm->tail.exact && !do_rec_match(lm->tail, tail, lt))
               return false;
         }
         break;
      default:
         throw vm_exec_error("can't match this argument");
   }
   return true;
}

static inline bool
do_matches(match* m, const tuple *tuple)
{
   if(!m->has_any_exact())
      return true;

   for(size_t i(0); i < tuple->num_fields(); ++i) {
      if(m->has_match(i)) {
         type *t = tuple->get_field_type(i);
         match_field mf(m->get_match(i));
         if(!do_rec_match(mf, tuple->get_field(i), t))
            return false;
      }
      
   }
   return true;
}

static void
build_match_element(instr_val val, match* m, type *t, match_field *mf, pcounter& pc, state& state)
{
   switch(t->get_type()) {
      case FIELD_INT:
         if(val_is_field(val)) {
            const reg_num reg(val_field_reg(pc));
            const tuple *tuple(state.get_tuple(reg));
            const field_num field(val_field_num(pc));

            pcounter_move_field(&pc);
            const int_val i(tuple->get_int(field));
            m->match_int(mf, i);
            const variable_match_template vmt = {reg, field, mf};
            m->add_variable_match(vmt);
         } else if(val_is_int(val)) {
            const int_val i(pcounter_int(pc));
            m->match_int(mf, i);
            pcounter_move_int(&pc);
         } else
            throw vm_exec_error("cannot use value for matching int");
         break;
      case FIELD_FLOAT: {
         if(val_is_field(val)) {
            const reg_num reg(val_field_reg(pc));
            const tuple *tuple(state.get_tuple(reg));
            const field_num field(val_field_num(pc));

            pcounter_move_field(&pc);
            const float_val f(tuple->get_float(field));
            m->match_float(mf, f);
            const variable_match_template vmt = {reg, field, mf};
            m->add_variable_match(vmt);
         } else if(val_is_float(val)) {
            const float_val f(pcounter_float(pc));
            m->match_float(mf, f);
            pcounter_move_float(&pc);
         } else
            throw vm_exec_error("cannot use value for matching float");
      }
      break;
      case FIELD_NODE:
         if(val_is_field(val)) {
            const reg_num reg(val_field_reg(pc));
            const tuple *tuple(state.get_tuple(reg));
            const field_num field(val_field_num(pc));

            pcounter_move_field(&pc);
            const node_val n(tuple->get_node(field));
            m->match_node(mf, n);
            const variable_match_template vmt = {reg, field, mf};
            m->add_variable_match(vmt);
         } else if(val_is_node(val)) {
            const node_val n(pcounter_node(pc));
            m->match_node(mf, n);
            pcounter_move_node(&pc);
         } else
            throw vm_exec_error("cannot use value for matching node");
         break;
      case FIELD_LIST:
         if(val_is_any(val)) {
            mf->exact = false;
            // do nothing
         } else if(val_is_nil(val)) {
            m->match_nil(mf);
         } else if(val_is_non_nil(val)) {
            m->match_non_nil(mf);
         } else if(val_is_list(val)) {
            list_type *lt((list_type*)t);
            list_match *lm(new list_match(lt));
            const instr_val head(val_get(pc, 0));
            pcounter_move_byte(&pc);
            if(!val_is_any(head))
               build_match_element(head, m, lt->get_subtype(), &(lm->head), pc, state);
            const instr_val tail(val_get(pc, 0));
            pcounter_move_byte(&pc);
            if(!val_is_any(tail))
               build_match_element(tail, m, t, &(lm->tail), pc, state);
            m->match_list(mf, lm, t);
         } else
            throw vm_exec_error("invalid field type for ITERATE/FIELD_LIST");
      break;
      default: throw vm_exec_error("invalid field type for ITERATE");
   }
}

static inline void
build_match_object(match* m, pcounter pc, const predicate *pred, state& state)
{
   if(iter_match_none(pc))
      return;
      
   iter_match match;
   type *t;
   
   do {
      match = pc;
      
      const field_num field(iter_match_field(match));
      const instr_val val(iter_match_val(match));
      
      pcounter_move_match(&pc);

      t = pred->get_field_type(field);
      build_match_element(val, m, t, m->get_update_match(field), pc, state);
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
execute_iter(const reg_num reg, match* m, pcounter pc, const utils::byte options, const utils::byte options_arguments,
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
   const depth_t old_depth = state.depth;                                  \
																					            \
	state.is_linear = this_is_linear || state.is_linear;                    \
   state.depth = !pred->is_cycle_pred() ? state.depth : max((NEW_DEPTH)+1, state.depth); \
   if((TUPLE_LEAF) != NULL) state.set_leaf(reg, (TUPLE_LEAF));  \
   if((TUPLE_QUEUE) != NULL) state.set_tuple_queue(reg, (TUPLE_QUEUE))
	
#define POP_STATE()								\
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
					if(do_matches(m, stpl->get_tuple()))
						everything.push_back(iter_object(ITER_LOCAL, (void*)stpl));
				}
			}
		}
		
		for(tuple_trie::tuple_search_iterator end(tuple_trie::match_end());
			tuples_it != end; ++tuples_it)
		{
			tuple_trie_leaf *tuple_leaf(*tuples_it);
#ifdef TRIE_MATCHING_ASSERT
	      assert(do_matches(m, tuple_leaf->get_underlying_tuple()));
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

			      ret = execute(first, state, reg, match_tuple);

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
					
					ret = execute(first, state, reg, match_tuple);
					
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

      while(true) {
         if(pred->is_linear_pred()) {
            if(state.linear_tuple_can_be_used(tuple_leaf) == 0)
               break;
            state.using_new_linear_tuple(tuple_leaf);
         }

         // we get the tuple later since the previous leaf may have been deleted
         tuple *match_tuple(tuple_leaf->get_underlying_tuple());
         assert(match_tuple != NULL);

#ifdef TRIE_MATCHING_ASSERT
         assert(do_matches(m, match_tuple));
#else
         (void)pc;
#endif

         PUSH_CURRENT_STATE(match_tuple, tuple_leaf, NULL, tuple_leaf->get_min_depth());

         return_type ret;

         ret = execute(first, state, reg, match_tuple);

         POP_STATE();

         if(ret == RETURN_LINEAR)
            return ret;

         if(ret == RETURN_DERIVED && state.is_linear) {
            return RETURN_DERIVED;
         }

         if(pred->is_linear_pred()) {
            if(ret != RETURN_LINEAR && ret != RETURN_DERIVED) {
               state.no_longer_using_linear_tuple(tuple_leaf); // not consumed because nothing was derived
               break; // exit while loop
            }

            if(!iter_options_to_delete(options)) {
               state.no_longer_using_linear_tuple(tuple_leaf); // cannot be consumed because it would get generated again
            }
         } else {
            break;
         }
      }
   }

	// current set of tuples
   if(!state.persistent_only) {
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
            if(!do_matches(m, match_tuple))
               continue;
         }
		
			PUSH_CURRENT_STATE(match_tuple, NULL, stpl, stpl->get_depth());
		
			if(iter_options_to_delete(options) || pred->is_linear_pred()) {
				stpl->will_delete(); // this will avoid future uses of this tuple!
			}
		
			// execute...
			return_type ret = execute(first, state, reg, match_tuple);
		
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
execute_testnil(pcounter pc, state& state)
{
   const reg_num op(test_nil_op(pc));
   const reg_num dest(test_nil_dest(pc));

   runtime::cons *x(state.get_cons(op));

   state.set_bool(dest, runtime::cons::is_null(x));
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
execute_float(pcounter& pc, state& state)
{
   const reg_num src(pcounter_reg(pc + instr_size));
   const reg_num dst(pcounter_reg(pc + instr_size + reg_val_size));

   state.set_float(dst, static_cast<float_val>(state.get_int(src)));
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
            mobj.match_int(mobj.get_update_match(fil_ind), idx);
            break;
         case FIELD_FLOAT:
            mobj.match_float(mobj.get_update_match(fil_ind), get_op_function<float_val>(fil_val, m, state));
            break;
         case FIELD_NODE:
            mobj.match_node(mobj.get_update_match(fil_ind), get_op_function<node_val>(fil_val, m, state));
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

#ifdef DEBUG_REMOVE
   cout << "\tdelete " << *tpl << endl;
#endif
   assert(tpl != NULL);
		
   if(is_a_leaf)
      state.node->matcher.deregister_tuple(tpl, 1);
   // the else case is done at state.cpp

   if(tpl->is_reused() && state.use_local_tuples) {
		state.generated_persistent_tuples.push_back(new simple_tuple(tpl, -1, state.depth));
		if(is_a_leaf)
			state.leaves_for_deletion.push_back(make_pair((predicate*)tpl->get_predicate(), state.get_leaf(reg)));
	} else {
		if(is_a_leaf) // tuple was fetched from database
			state.leaves_for_deletion.push_back(make_pair((predicate*)tpl->get_predicate(), state.get_leaf(reg)));
		else {
			// tuple was marked before, it will be deleted after this round
		}
	}
}

static inline void
set_call_return(const reg_num reg, const tuple_field ret, external_function* f, state& state)
{
   type *ret_type(f->get_return_type());
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
         throw vm_exec_error("invalid return type in call (set_call_return)");
   }
}

static inline void
execute_call0(pcounter& pc, state& state)
{
   const external_function_id id(call_extern_id(pc));
   external_function *f(lookup_external_function(id));

   assert(f->get_num_args() == 0);

   argument ret = f->get_fun_ptr()();
   set_call_return(call_dest(pc), ret, f, state);
}

static inline void
execute_call1(pcounter& pc, state& state)
{
   const external_function_id id(call_extern_id(pc));
   external_function *f(lookup_external_function(id));

   assert(f->get_num_args() == 1);

   argument ret = ((external_function_ptr1)f->get_fun_ptr())(state.get_reg(pcounter_reg(pc + call_size)));
   set_call_return(call_dest(pc), ret, f, state);
}

static inline void
execute_call2(pcounter& pc, state& state)
{
   const external_function_id id(call_extern_id(pc));
   external_function *f(lookup_external_function(id));

   assert(f->get_num_args() == 2);

   argument ret = ((external_function_ptr2)f->get_fun_ptr())(state.get_reg(pcounter_reg(pc + call_size)),
         state.get_reg(pcounter_reg(pc + call_size + reg_val_size)));
   set_call_return(call_dest(pc), ret, f, state);
}

static inline void
execute_call3(pcounter& pc, state& state)
{
   const external_function_id id(call_extern_id(pc));
   external_function *f(lookup_external_function(id));

   assert(f->get_num_args() == 3);

   argument ret = ((external_function_ptr3)f->get_fun_ptr())(state.get_reg(pcounter_reg(pc + call_size)),
         state.get_reg(pcounter_reg(pc + call_size + reg_val_size)),
         state.get_reg(pcounter_reg(pc + call_size + 2 * reg_val_size)));
   set_call_return(call_dest(pc), ret, f, state);
}

static inline void
execute_call(pcounter& pc, state& state)
{
   const external_function_id id(call_extern_id(pc));
   external_function *f(lookup_external_function(id));
   const size_t num_args(call_num_args(pc));
   argument args[num_args];
   
   pcounter m(pc + CALL_BASE);
   for(size_t i(0); i < num_args; ++i) {
      args[i] = state.get_reg(pcounter_reg(m));
      m += reg_val_size;
   }
   
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

   set_call_return(call_dest(pc), ret, f, state);
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

static inline void
execute_mvintfield(pcounter pc, state& state)
{
   tuple *tuple(get_tuple_field(state, pc + instr_size + int_size));
   
   tuple->set_int(val_field_num(pc + instr_size + int_size), pcounter_int(pc + instr_size));
}

static inline void
execute_mvintreg(pcounter pc, state& state)
{
   state.set_int(pcounter_reg(pc + instr_size + int_size), pcounter_int(pc + instr_size));
}

static inline void
execute_mvfieldfield(pcounter pc, state& state)
{
   tuple *tuple_from(get_tuple_field(state, pc + instr_size));
   tuple *tuple_to(get_tuple_field(state, pc + instr_size + field_size));
   const field_num from(val_field_num(pc + instr_size));
   const field_num to(val_field_num(pc + instr_size + field_size));
   tuple_to->set_field(to, tuple_from->get_field(from));
   assert(!reference_type(tuple_to->get_field_type(to)->get_type()));
}
static inline void
execute_mvfieldfieldr(pcounter pc, state& state)
{
   tuple *tuple_from(get_tuple_field(state, pc + instr_size));
   tuple *tuple_to(get_tuple_field(state, pc + instr_size + field_size));
   const field_num from(val_field_num(pc + instr_size));
   const field_num to(val_field_num(pc + instr_size + field_size));
   tuple_to->set_field(to, tuple_from->get_field(from));

   assert(reference_type(tuple_to->get_field_type(to)->get_type()));

   runtime::ref_base *p((runtime::ref_base*)tuple_from->get_field(from).ptr_field);

   if(p)
      p->refs++;
}

static inline void
execute_mvfieldreg(pcounter pc, state& state)
{
   tuple *tuple_from(get_tuple_field(state, pc + instr_size));
   const field_num from(val_field_num(pc + instr_size));

   state.set_reg(pcounter_reg(pc + instr_size + field_size), tuple_from->get_field(from));
}

static inline void
execute_mvptrreg(pcounter pc, state& state)
{
   const ptr_val p(pcounter_ptr(pc + instr_size));
   const reg_num reg(pcounter_reg(pc + instr_size + ptr_size));

   state.set_ptr(reg, p);
}

static inline void
execute_mvnilfield(pcounter& pc, state& state)
{
   tuple *tuple(get_tuple_field(state, pc + instr_size));

   tuple->set_nil(val_field_num(pc + instr_size));
}

static inline void
execute_mvnilreg(pcounter& pc, state& state)
{
   state.set_nil(pcounter_reg(pc + instr_size));
}

static inline void
execute_mvregfield(pcounter& pc, state& state)
{
   const reg_num reg(pcounter_reg(pc + instr_size));
   tuple *tuple(get_tuple_field(state, pc + instr_size + reg_val_size));
   const field_num field(val_field_num(pc + instr_size + reg_val_size));
   tuple->set_field(field, state.get_reg(reg));
   assert(!reference_type(tuple->get_field_type(field)->get_type()));
}

static inline void
execute_mvregfieldr(pcounter& pc, state& state)
{
   const reg_num reg(pcounter_reg(pc + instr_size));
   tuple *tuple(get_tuple_field(state, pc + instr_size + reg_val_size));
   const field_num field(val_field_num(pc + instr_size + reg_val_size));
   tuple->set_field(field, state.get_reg(reg));

   assert(reference_type(tuple->get_field_type(field)->get_type()));

   runtime::ref_base *p((runtime::ref_base*)tuple->get_field(field).ptr_field);

   if(p)
      p->refs++;
}

static inline void
execute_mvhostfield(pcounter& pc, state& state)
{
   tuple *tuple(get_tuple_field(state, pc + instr_size));
   const field_num field(val_field_num(pc + instr_size));

   tuple->set_node(field, state.node->get_id());
}

static inline void
execute_mvregconst(pcounter& pc, state& state)
{
   const const_id id(pcounter_const_id(pc + instr_size + reg_val_size));
   state.all->set_const(id, state.get_reg(pcounter_reg(pc + instr_size)));
   if(reference_type(state.all->PROGRAM->get_const_type(id)->get_type())) {
      runtime::ref_base *p((runtime::ref_base*)state.all->get_const(id).ptr_field);

      if(p)
         p->refs++;
   }
}

static inline void
execute_mvconstfield(pcounter& pc, state& state)
{
   const const_id id(pcounter_const_id(pc + instr_size));
   const field_num field(val_field_num(pc + instr_size + const_id_size));
   tuple *tuple(get_tuple_field(state, pc + instr_size + const_id_size));

   tuple->set_field(field, state.all->get_const(id));
   assert(!reference_type(tuple->get_field_type(field)->get_type()));
}

static inline void
execute_mvconstfieldr(pcounter& pc, state& state)
{
   const const_id id(pcounter_const_id(pc + instr_size));
   const field_num field(val_field_num(pc + instr_size + const_id_size));
   tuple *tuple(get_tuple_field(state, pc + instr_size + const_id_size));

   tuple->set_field(field, state.all->get_const(id));
   assert(reference_type(tuple->get_field_type(field)->get_type()));
   runtime::ref_base *p((runtime::ref_base*)tuple->get_field(field).ptr_field);

   if(p)
      p->refs++;
}

static inline void
execute_mvconstreg(pcounter& pc, state& state)
{
   const const_id id(pcounter_const_id(pc + instr_size));
   const reg_num reg(pcounter_reg(pc + instr_size + const_id_size));

   state.set_reg(reg, state.all->get_const(id));
}

static inline void
execute_mvaddrfield(pcounter& pc, state& state)
{
   const node_val n(pcounter_node(pc + instr_size));
   const field_num field(val_field_num(pc + instr_size + node_size));
   tuple *tuple(get_tuple_field(state, pc + instr_size + node_size));

   tuple->set_node(field, n);
}

static inline void
execute_mvfloatfield(pcounter& pc, state& state)
{
   const float_val f(pcounter_float(pc + instr_size));
   const field_num field(val_field_num(pc + instr_size + float_size));
   tuple *tuple(get_tuple_field(state, pc + instr_size + float_size));

   tuple->set_float(field, f);
}

static inline void
execute_mvfloatreg(pcounter& pc, state& state)
{
   const float_val f(pcounter_float(pc + instr_size));
   const reg_num reg(pcounter_reg(pc + instr_size + float_size));

   state.set_float(reg, f);
}

static inline void
execute_mvintconst(pcounter& pc, state& state)
{
   const int_val i(pcounter_int(pc + instr_size));
   const const_id id(pcounter_const_id(pc + instr_size + int_size));

   state.all->set_const_int(id, i);
}

static inline void
execute_mvworldfield(pcounter& pc, state& state)
{
   const field_num field(val_field_num(pc + instr_size));
   tuple *tuple(get_tuple_field(state, pc + instr_size));

   tuple->set_int(field, state.all->DATABASE->nodes_total);
}

static inline void
execute_mvworldreg(pcounter& pc, state& state)
{
   const reg_num reg(pcounter_reg(pc + instr_size));

   state.set_int(reg, state.all->DATABASE->nodes_total);
}

static inline void
execute_mvstackpcounter(pcounter& pc, state& state)
{
   const offset_num off(pcounter_stack(pc + instr_size));

   pc = (pcounter)FIELD_PCOUNTER(*(state.get_stack_at(off)));
}

static inline void
execute_mvpcounterstack(pcounter& pc, state& state)
{
   const offset_num off(pcounter_stack(pc + instr_size));

   SET_FIELD_PTR(*(state.get_stack_at(off)), pc + MVPCOUNTERSTACK_BASE);
}

static inline void
execute_mvstackreg(pcounter& pc, state& state)
{
   const offset_num off(pcounter_stack(pc + instr_size));
   const reg_num reg(pcounter_reg(pc + instr_size + stack_val_size));

   state.set_reg(reg, *state.get_stack_at(off));
}

static inline void
execute_mvregstack(pcounter& pc, state& state)
{
   const reg_num reg(pcounter_reg(pc + instr_size));
   const offset_num off(pcounter_stack(pc + instr_size + reg_val_size));

   *(state.get_stack_at(off)) = state.get_reg(reg);
}

static inline void
execute_mvaddrreg(pcounter& pc, state& state)
{
   const node_val n(pcounter_node(pc + instr_size));
   const reg_num reg(pcounter_reg(pc + instr_size + node_size));

   state.set_node(reg, n);
}

static inline void
execute_mvhostreg(pcounter& pc, state& state)
{
   const reg_num reg(pcounter_reg(pc + instr_size));
   state.set_node(reg, state.node->get_id());
}

static inline void
execute_mvregreg(pcounter& pc, state& state)
{
   state.copy_reg(pcounter_reg(pc + instr_size), pcounter_reg(pc + instr_size + reg_val_size));
}

#define DO_OPERATION(SET_FUNCTION, GET_FUNCTION, OP)                    \
   const reg_num op1(pcounter_reg(pc + instr_size));                    \
   const reg_num op2(pcounter_reg(pc + instr_size + reg_val_size));     \
   const reg_num dst(pcounter_reg(pc + instr_size + 2 * reg_val_size)); \
   state.SET_FUNCTION(dst, state.GET_FUNCTION(op1) OP state.GET_FUNCTION(op2))
#define BOOL_OPERATION(GET_FUNCTION, OP)                                \
   DO_OPERATION(set_bool, GET_FUNCTION, OP)

static inline void
execute_addrnotequal(pcounter& pc, state& state)
{
   BOOL_OPERATION(get_node, !=);
}

static inline void
execute_addrequal(pcounter& pc, state& state)
{
   BOOL_OPERATION(get_node, ==);
}

static inline void
execute_intminus(pcounter& pc, state& state)
{
   DO_OPERATION(set_int, get_int, -);
}

static inline void
execute_intequal(pcounter& pc, state& state)
{
   DO_OPERATION(set_int, get_int, ==);
}

static inline void
execute_intnotequal(pcounter& pc, state& state)
{
   DO_OPERATION(set_int, get_int, !=);
}

static inline void
execute_intplus(pcounter& pc, state& state)
{
   DO_OPERATION(set_int, get_int, +);
}

static inline void
execute_intlesser(pcounter& pc, state& state)
{
   DO_OPERATION(set_bool, get_int, <);
}

static inline void
execute_intgreaterequal(pcounter& pc, state& state)
{
   DO_OPERATION(set_bool, get_int, >=);
}

static inline void
execute_boolor(pcounter& pc, state& state)
{
   DO_OPERATION(set_bool, get_bool, ||);
}

static inline void
execute_intlesserequal(pcounter& pc, state& state)
{
   DO_OPERATION(set_bool, get_int, <=);
}

static inline void
execute_intgreater(pcounter& pc, state& state)
{
   DO_OPERATION(set_bool, get_int, >);
}

static inline void
execute_intmul(pcounter& pc, state& state)
{
   DO_OPERATION(set_int, get_int, *);
}

static inline void
execute_intdiv(pcounter& pc, state& state)
{
   DO_OPERATION(set_int, get_int, /);
}

static inline void
execute_floatplus(pcounter& pc, state& state)
{
   DO_OPERATION(set_float, get_float, +);
}

static inline void
execute_floatminus(pcounter& pc, state& state)
{
   DO_OPERATION(set_float, get_float, -);
}

static inline void
execute_floatmul(pcounter& pc, state& state)
{
   DO_OPERATION(set_float, get_float, *);
}

static inline void
execute_floatdiv(pcounter& pc, state& state)
{
   DO_OPERATION(set_float, get_float, /);
}

static inline void
execute_floatequal(pcounter& pc, state& state)
{
   DO_OPERATION(set_bool, get_float, ==);
}

static inline void
execute_floatnotequal(pcounter& pc, state& state)
{
   DO_OPERATION(set_bool, get_float, !=);
}

static inline void
execute_floatlesser(pcounter& pc, state& state)
{
   DO_OPERATION(set_bool, get_float, <);
}

static inline void
execute_floatlesserequal(pcounter& pc, state& state)
{
   DO_OPERATION(set_bool, get_float, <=);
}

static inline void
execute_floatgreater(pcounter& pc, state& state)
{
   DO_OPERATION(set_bool, get_float, >);
}

static inline void
execute_floatgreaterequal(pcounter& pc, state& state)
{
   DO_OPERATION(set_bool, get_float, >=);
}

static inline void
execute_boolequal(pcounter& pc, state& state)
{
   DO_OPERATION(set_bool, get_bool, ==);
}

static inline void
execute_boolnotequal(pcounter& pc, state& state)
{
   DO_OPERATION(set_bool, get_bool, !=);
}

static inline void
execute_headrr(pcounter& pc, state& state)
{
   const reg_num ls(pcounter_reg(pc + instr_size));
   const reg_num dst(pcounter_reg(pc + instr_size + reg_val_size));

   runtime::cons *l(state.get_cons(ls));

   state.set_reg(dst, l->get_head());
}

static inline void
execute_headfr(pcounter& pc, state& state)
{
   tuple *tuple(get_tuple_field(state, pc + instr_size));
   const field_num field(val_field_num(pc + instr_size));
   runtime::cons *l(tuple->get_cons(field));
   const reg_num dst(pcounter_reg(pc + instr_size + field_size));

   state.set_reg(dst, l->get_head());
}

static inline void
execute_headff(pcounter& pc, state& state)
{
   tuple *tsrc(get_tuple_field(state, pc + instr_size));
   const field_num src(val_field_num(pc + instr_size));
   tuple *tdst(get_tuple_field(state, pc + instr_size + field_size));
   const field_num dst(val_field_num(pc + instr_size + field_size));

   runtime::cons *l(tsrc->get_cons(src));

   tdst->set_field(dst, l->get_head());
   assert(!reference_type(tdst->get_field_type(dst)->get_type()));
}

static inline void
execute_headffr(pcounter& pc, state& state)
{
   tuple *tsrc(get_tuple_field(state, pc + instr_size));
   const field_num src(val_field_num(pc + instr_size));
   tuple *tdst(get_tuple_field(state, pc + instr_size + field_size));
   const field_num dst(val_field_num(pc + instr_size + field_size));

   runtime::cons *l(tsrc->get_cons(src));

   tdst->set_field(dst, l->get_head());
   assert(reference_type(tdst->get_field_type(dst)->get_type()));
   runtime::ref_base *p((runtime::ref_base*)tdst->get_field(dst).ptr_field);

   if(p)
      p->refs++;
}

static inline void
execute_headrf(pcounter& pc, state& state)
{
   const reg_num reg(pcounter_reg(pc + instr_size));
   tuple *tdst(get_tuple_field(state, pc + instr_size + reg_val_size));
   const field_num dst(val_field_num(pc + instr_size + reg_val_size));

   runtime::cons *l(state.get_cons(reg));

   tdst->set_field(dst, l->get_head());
   assert(!reference_type(tdst->get_field_type(dst)->get_type()));
}

static inline void
execute_headrfr(pcounter& pc, state& state)
{
   const reg_num reg(pcounter_reg(pc + instr_size));
   tuple *tdst(get_tuple_field(state, pc + instr_size + reg_val_size));
   const field_num dst(val_field_num(pc + instr_size + reg_val_size));

   runtime::cons *l(state.get_cons(reg));

   tdst->set_field(dst, l->get_head());

   assert(reference_type(tdst->get_field_type(dst)->get_type()));

   runtime::ref_base *p((runtime::ref_base*)tdst->get_field(dst).ptr_field);

   if(p)
      p->refs++;
}

static inline void
execute_tailrr(pcounter& pc, state& state)
{
   const reg_num src(pcounter_reg(pc + instr_size));
   const reg_num dst(pcounter_reg(pc + instr_size + reg_val_size));

   runtime::cons *l(state.get_cons(src));

   state.set_cons(dst, l->get_tail());
}

static inline void
execute_tailfr(pcounter& pc, state& state)
{
   tuple *tuple(get_tuple_field(state, pc + instr_size));
   const field_num field(val_field_num(pc + instr_size));
   const reg_num dst(pcounter_reg(pc + instr_size + field_size));

   state.set_cons(dst, tuple->get_cons(field)->get_tail());
}

static inline void
execute_tailff(pcounter& pc, state& state)
{
   tuple *tsrc(get_tuple_field(state, pc + instr_size));
   const field_num src(val_field_num(pc + instr_size));
   tuple *tdst(get_tuple_field(state, pc + instr_size + field_size));
   const field_num dst(val_field_num(pc + instr_size + field_size));

   tdst->set_cons(dst, tsrc->get_cons(src)->get_tail());
}

static inline void
execute_tailrf(pcounter& pc, state& state)
{
   const reg_num src(pcounter_reg(pc + instr_size));
   tuple *tuple(get_tuple_field(state, pc + instr_size + reg_val_size));
   const field_num field(val_field_num(pc + instr_size + reg_val_size));

   tuple->set_cons(field, state.get_cons(src)->get_tail());
}

static inline void
execute_consrrr(pcounter& pc, state& state)
{
   const reg_num head(pcounter_reg(pc + instr_size + type_size));
   const reg_num tail(pcounter_reg(pc + instr_size + type_size + reg_val_size));
   const reg_num dest(pcounter_reg(pc + instr_size + type_size + 2 * reg_val_size));

   list_type *ltype((list_type*)state.all->PROGRAM->get_type(cons_type(pc)));
   cons *new_list(new cons(state.get_cons(tail), state.get_reg(head), ltype));
	state.add_cons(new_list);
   state.set_cons(dest, new_list);
}

static inline void
execute_consrff(pcounter& pc, state& state)
{
   const reg_num head(pcounter_reg(pc + instr_size));
   tuple *tail(get_tuple_field(state, pc + instr_size + reg_val_size));
   const field_num tail_field(val_field_num(pc + instr_size + reg_val_size));
   tuple *dest(get_tuple_field(state, pc + instr_size + reg_val_size + field_size));
   const field_num dest_field(val_field_num(pc + instr_size + reg_val_size + field_size));

   cons *new_list(new cons(tail->get_cons(tail_field), state.get_reg(head), (list_type*)tail->get_field_type(tail_field)));
   dest->set_cons(dest_field, new_list);
}

static inline void
execute_consfrf(pcounter& pc, state& state)
{
   tuple *head(get_tuple_field(state, pc + instr_size));
   const field_num head_field(val_field_num(pc + instr_size));
   const reg_num tail(pcounter_reg(pc + instr_size + field_size));
   tuple *dest(get_tuple_field(state, pc + instr_size + field_size + reg_val_size));
   const field_num dest_field(val_field_num(pc + instr_size + field_size + reg_val_size));

   cons *new_list(new cons(state.get_cons(tail), head->get_field(head_field), (list_type*)dest->get_field_type(dest_field)));
   dest->set_cons(dest_field, new_list);
}

static inline void
execute_consffr(pcounter& pc, state& state)
{
   tuple *head(get_tuple_field(state, pc + instr_size));
   const field_num head_field(val_field_num(pc + instr_size));
   tuple *tail(get_tuple_field(state, pc + instr_size + field_size));
   const field_num tail_field(val_field_num(pc + instr_size + field_size));
   const reg_num dest(pcounter_reg(pc + instr_size + 2 * field_size));

   cons *new_list(new cons(tail->get_cons(tail_field), head->get_field(head_field), (list_type*)tail->get_field_type(tail_field)));
	state.add_cons(new_list);
   state.set_cons(dest, new_list);
}

static inline void
execute_consrrf(pcounter& pc, state& state)
{
   const reg_num head(pcounter_reg(pc + instr_size));
   const reg_num tail(pcounter_reg(pc + instr_size + reg_val_size));
   tuple *dest(get_tuple_field(state, pc + instr_size + 2 * reg_val_size));
   const field_num field(val_field_num(pc + instr_size + 2 * reg_val_size));

   cons *new_list(new cons(state.get_cons(tail), state.get_reg(head), (list_type*)dest->get_field_type(field)));
   dest->set_cons(field, new_list);
}

static inline void
execute_consrfr(pcounter& pc, state& state)
{
   const reg_num head(pcounter_reg(pc + instr_size));
   tuple *tail(get_tuple_field(state, pc + instr_size + reg_val_size));
   const field_num field(val_field_num(pc + instr_size + reg_val_size));
   const reg_num dest(pcounter_reg(pc + instr_size + reg_val_size + field_size));

   cons *new_list(new cons(tail->get_cons(field), state.get_reg(head), (list_type*)tail->get_field_type(field)));
   state.add_cons(new_list);
   state.set_cons(dest, new_list);
}

static inline void
execute_consfrr(pcounter& pc, state& state)
{
   tuple *head(get_tuple_field(state, pc + instr_size + type_size));
   const field_num field(val_field_num(pc + instr_size + type_size));
   const reg_num tail(pcounter_reg(pc + instr_size + type_size + field_size));
   const reg_num dest(pcounter_reg(pc + instr_size + type_size + field_size + reg_val_size));

   list_type *ltype((list_type*)state.all->PROGRAM->get_type(cons_type(pc)));
   cons *new_list(new cons(state.get_cons(tail), head->get_field(field), ltype));
   state.add_cons(new_list);
   state.set_cons(dest, new_list);
}

static inline void
execute_consfff(pcounter& pc, state& state)
{
   tuple *head(get_tuple_field(state, pc + instr_size));
   const field_num field_head(val_field_num(pc + instr_size));
   tuple *tail(get_tuple_field(state, pc + instr_size + field_size));
   const field_num field_tail(val_field_num(pc + instr_size + field_size));
   tuple *dest(get_tuple_field(state, pc + instr_size + 2 * field_size));
   const field_num field_dest(val_field_num(pc + instr_size + 2 * field_size));

   cons *new_list(new cons(tail->get_cons(field_tail), head->get_field(field_head), (list_type*)tail->get_field_type(field_tail)));
   dest->set_cons(field_dest, new_list);
}

static inline return_type
execute(pcounter pc, state& state, const reg_num reg, tuple *tpl)
{
	if(tpl != NULL) {
      state.set_tuple(reg, tpl);
#ifdef CORE_STATISTICS
		state.stat.stat_tuples_used++;
      if(tpl->is_linear()) {
         state.stat.stat_predicate_applications[tpl->get_predicate_id()]++;
      }
#endif
   }

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
         
			case END_LINEAR_INSTR:
				return RETURN_END_LINEAR;
			
         case RESET_LINEAR_INSTR:
            {
               const bool old_is_linear(state.is_linear);
               
               state.is_linear = false;
               
               return_type ret(execute(pc + RESET_LINEAR_BASE, state, 0, NULL));

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
               const reg_num reg(iter_reg(pc));

               match *mobj(NULL);
               vm::state::match_store_type::iterator match_found(state.match_store.find(pc));
               if(match_found == state.match_store.end()) {
                  mobj = new match(pred);
                  build_match_object(mobj, pc + ITER_BASE, pred, state);
                  state.match_store[pc] = mobj;
               } else {
                  mobj = match_found->second;
                  if(!iter_options_const(iter_options(pc))) {
                     for(size_t i(0); i < mobj->variable_matches.size(); ++i) {
                        variable_match_template& tmp(mobj->variable_matches[i]);
                        tuple *tpl(state.get_tuple(tmp.reg));
                        tmp.match->field = tpl->get_field(tmp.field);
                     }
                  }
               }
               assert(mobj);
               
#ifdef CORE_STATISTICS
					state.stat.stat_db_hits++;
#endif
#ifdef CORE_STATISTICS
                  //execution_time::scope s(state.stat.db_search_time_predicate[pred_id]);
#endif
               tuple_trie::tuple_search_iterator it = state.node->match_predicate(pred_id, mobj);

               const return_type ret(execute_iter(reg, mobj, pc + ITER_BASE,
								iter_options(pc), iter_options_argument(pc),
								pc + iter_inner_jump(pc), state, it, pred));
                  
               if(ret == RETURN_LINEAR)
                 return ret;
					if(state.is_linear && ret == RETURN_DERIVED)
						return ret;
               
               pc += iter_outer_jump(pc);
               goto eval_loop;
            }
            
         case REMOVE_INSTR:
            execute_remove(pc, state);
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
            
         case NOT_INSTR:
            execute_not(pc, state);
            break;
            
         case TESTNIL_INSTR:
            execute_testnil(pc, state);
            break;
            
         case FLOAT_INSTR:
            execute_float(pc, state);
            break;
            
         case SELECT_INSTR:
            pc = execute_select(pc, state);
            goto eval_loop;
            
         case DELETE_INSTR:
            execute_delete(pc, state);
            break;
            
         case CALL_INSTR:
            execute_call(pc, state);
            break;
         case CALL0_INSTR:
            execute_call0(pc, state);
            break;
         case CALL1_INSTR:
            execute_call1(pc, state);
            break;
         case CALL2_INSTR:
            execute_call2(pc, state);
            break;
         case CALL3_INSTR:
            execute_call3(pc, state);
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
           state.stack.push_front(tuple_field());
           break;

         case POP_INSTR:
           state.stack.pop_front();
           break;

         case PUSH_REGS_INSTR:
           state.stack.insert(state.stack.begin(),
                 state.regs, state.regs + NUM_REGS);
           break;

         case POP_REGS_INSTR:
           copy(state.stack.begin(), state.stack.begin() + NUM_REGS, state.regs);
           state.stack.erase(state.stack.begin(), state.stack.begin() + NUM_REGS);
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

         case MVINTFIELD_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvintfield(pc, state);
           break;

         case MVINTREG_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvintreg(pc, state);
           break;

         case MVFIELDFIELD_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvfieldfield(pc, state);
           break;

         case MVFIELDFIELDR_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvfieldfieldr(pc, state);
           break;

         case MVFIELDREG_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvfieldreg(pc, state);
           break;

         case MVPTRREG_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvptrreg(pc, state);
           break;

         case MVNILFIELD_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvnilfield(pc, state);
           break;

         case MVNILREG_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvnilreg(pc, state);
           break;

         case MVREGFIELD_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvregfield(pc, state);
           break;

         case MVREGFIELDR_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvregfieldr(pc, state);
           break;

         case MVHOSTFIELD_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvhostfield(pc, state);
           break;

         case MVREGCONST_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvregconst(pc, state);
           break;

         case MVCONSTFIELD_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvconstfield(pc, state);
           break;

         case MVCONSTFIELDR_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvconstfieldr(pc, state);
           break;

         case MVADDRFIELD_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvaddrfield(pc, state);
           break;

         case MVFLOATFIELD_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvfloatfield(pc, state);
           break;

         case MVFLOATREG_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvfloatreg(pc, state);
           break;

         case MVINTCONST_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvintconst(pc, state);
           break;

         case MVWORLDFIELD_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvworldfield(pc, state);
           break;

         case MVSTACKPCOUNTER_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvstackpcounter(pc, state);
           break;

         case MVPCOUNTERSTACK_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvpcounterstack(pc, state);
           break;

         case MVSTACKREG_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvstackreg(pc, state);
           break;

         case MVREGSTACK_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvregstack(pc, state);
           break;

         case MVADDRREG_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvaddrreg(pc, state);
           break;

         case MVHOSTREG_INSTR:
#ifdef CORE_STATISTICS
           state.stat.stat_moves_executed++;
#endif
           execute_mvhostreg(pc, state);
           break;

         case ADDRNOTEQUAL_INSTR:
           execute_addrnotequal(pc, state);
           break;

         case ADDREQUAL_INSTR:
           execute_addrequal(pc, state);
           break;

         case INTMINUS_INSTR:
           execute_intminus(pc, state);
           break;

         case INTEQUAL_INSTR:
           execute_intequal(pc, state);
           break;

         case INTNOTEQUAL_INSTR:
           execute_intnotequal(pc, state);
           break;

         case INTPLUS_INSTR:
           execute_intplus(pc, state);
           break;

         case INTLESSER_INSTR:
           execute_intlesser(pc, state);
           break;

         case INTGREATEREQUAL_INSTR:
           execute_intgreaterequal(pc, state);
           break;

         case BOOLOR_INSTR:
           execute_boolor(pc, state);
           break;

         case INTLESSEREQUAL_INSTR:
           execute_intlesserequal(pc, state);
           break;

         case INTGREATER_INSTR:
           execute_intgreater(pc, state);
           break;

         case INTMUL_INSTR:
           execute_intmul(pc, state);
           break;

         case INTDIV_INSTR:
           execute_intdiv(pc, state);
           break;

         case FLOATPLUS_INSTR:
           execute_floatplus(pc, state);
           break;

         case FLOATMINUS_INSTR:
           execute_floatminus(pc, state);
           break;

         case FLOATMUL_INSTR:
           execute_floatmul(pc, state);
           break;

         case FLOATDIV_INSTR:
           execute_floatdiv(pc, state);
           break;

         case FLOATEQUAL_INSTR:
           execute_floatequal(pc, state);
           break;

         case FLOATNOTEQUAL_INSTR:
           execute_floatnotequal(pc, state);
           break;

         case FLOATLESSER_INSTR:
           execute_floatlesser(pc, state);
           break;

         case FLOATLESSEREQUAL_INSTR:
           execute_floatlesserequal(pc, state);
           break;

         case FLOATGREATER_INSTR:
           execute_floatgreater(pc, state);
           break;

         case FLOATGREATEREQUAL_INSTR:
           execute_floatgreaterequal(pc, state);
           break;

         case MVREGREG_INSTR:
           execute_mvregreg(pc, state);
           break;

         case BOOLEQUAL_INSTR:
           execute_boolequal(pc, state);
           break;

         case BOOLNOTEQUAL_INSTR:
           execute_boolnotequal(pc, state);
           break;

         case HEADRR_INSTR:
           execute_headrr(pc, state);
           break;

         case HEADFR_INSTR:
           execute_headfr(pc, state);
           break;

         case HEADFF_INSTR:
           execute_headff(pc, state);
           break;

         case HEADRF_INSTR:
           execute_headrf(pc, state);
           break;

         case HEADFFR_INSTR:
           execute_headffr(pc, state);
           break;

         case HEADRFR_INSTR:
           execute_headrfr(pc, state);
           break;

         case TAILRR_INSTR:
           execute_tailrr(pc, state);
           break;

         case TAILFR_INSTR:
           execute_tailfr(pc, state);
           break;

         case TAILFF_INSTR:
           execute_tailff(pc, state);
           break;

         case TAILRF_INSTR:
           execute_tailrf(pc, state);
           break;

         case MVWORLDREG_INSTR:
           execute_mvworldreg(pc, state);
           break;

         case MVCONSTREG_INSTR:
           execute_mvconstreg(pc, state);
           break;

         case CONSRRR_INSTR:
           execute_consrrr(pc, state);
           break;

         case CONSRFF_INSTR:
           execute_consrff(pc, state);
           break;

         case CONSFRF_INSTR:
           execute_consfrf(pc, state);
           break;

         case CONSFFR_INSTR:
           execute_consffr(pc, state);
           break;

         case CONSRRF_INSTR:
           execute_consrrf(pc, state);
           break;

         case CONSRFR_INSTR:
           execute_consrfr(pc, state);
           break;

         case CONSFRR_INSTR:
           execute_consfrr(pc, state);
           break;

         case CONSFFF_INSTR:
           execute_consfff(pc, state);
           break;

         default: throw vm_exec_error("unsupported instruction");
      }
   }
}

static inline return_type
do_execute(byte_code code, state& state, const reg_num reg, tuple *tpl)
{
   assert(state.stack.empty());
   const return_type ret(execute((pcounter)code, state, reg, tpl));

   state.cleanup();
   assert(state.stack.empty());
   return ret;
}
   
execution_return
execute_bytecode(byte_code code, state& state, tuple *tpl)
{
   const return_type ret(do_execute(code, state, 0, tpl));
	
#ifdef CORE_STATISTICS
#endif
   
   if(ret == RETURN_LINEAR)
      return EXECUTION_CONSUMED;
	else
      return EXECUTION_OK;
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

   do_execute(rule->get_bytecode(), state, 0, NULL);

#ifdef CORE_STATISTICS
   if(state.stat.stat_rules_activated == 0)
      state.stat.stat_rules_failed++;
#endif

}

}
