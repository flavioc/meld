
#include <iostream>
#include <algorithm>
#include <functional>
#include <cmath>
#include <sstream>

#include "interface.hpp"
#include "vm/exec.hpp"
#include "vm/tuple.hpp"
#include "vm/match.hpp"
#include "vm/full_tuple.hpp"
#include "machine.hpp"
#include "utils/mutex.hpp"
#include "thread/thread.hpp"
#include "vm/priority.hpp"
#include "db/array.hpp"

#ifndef COMPILED

//#define DEBUG_INSTRS

#ifndef DEBUG_INSTRS
#define COMPUTED_GOTOS
#endif

#if defined(DEBUG_SENDS)
static utils::mutex print_mtx;
#endif

using namespace vm;
using namespace vm::instr;
using namespace std;
using namespace db;
using namespace runtime;
using namespace utils;

namespace vm {

enum return_type {
   RETURN_OK,
   RETURN_SELECT,
   RETURN_NEXT,
   RETURN_LINEAR,
   RETURN_DERIVED,
   RETURN_END_LINEAR,
   RETURN_NO_RETURN
};

static inline return_type execute(pcounter, state&, const reg_num, tuple*,
                                  predicate*);

static inline tuple* get_tuple_field(state& state, const pcounter& pc) {
   return state.get_tuple(val_field_reg(pc));
}

#include "vm/helpers.cpp"

static inline void execute_alloc(const pcounter& pc, state& state) {
   predicate* pred(theProgram->get_predicate(alloc_predicate(pc)));
   const reg_num dest(alloc_dest(pc));
   const reg_num reg(alloc_reg(pc));
   mem::node_allocator *alloc;
   db::node *target_node{nullptr};
   if(pred->is_thread_pred()) {
      if(dest == reg) {
         target_node = state.sched->thread_node;
         alloc = &(target_node->alloc);
      } else {
         sched::thread *s((sched::thread*)state.get_thread(dest));
         alloc = &(s->thread_node->alloc);
      }
   } else {
      if(dest == reg) {
         target_node = state.node;
         alloc = &(target_node->alloc);
      } else
         alloc = &(((db::node*)state.get_node(dest))->alloc);
   }
   tuple *tpl;
   if(pred->is_compact_pred()) {
      assert(dest == reg);
      assert(target_node);
      tpl = target_node->pers_store.get_array(pred)->expand(pred, alloc);
   } else
      tpl = vm::tuple::create(pred, alloc);

   state.preds[reg] = pred;

   state.set_tuple(reg, tpl);

#ifdef INSTRUMENTATION
   state.instr_facts_derived++;
#endif
}

static inline void execute_add_linear(db::node* node, pcounter& pc,
                                      state& state) {
   const reg_num r(pcounter_reg(pc + instr_size));
   predicate* pred(state.preds[r]);
   tuple* tuple(state.get_tuple(r));
   assert(!pred->is_reused_pred());
   assert(!pred->is_action_pred());
   assert(pred->is_linear_pred());

   execute_add_linear0(node, tuple, pred, state);
}

static inline void execute_add_node_persistent(db::node* node, pcounter& pc,
                                               state& state) {
   const reg_num r(pcounter_reg(pc + instr_size));

   // tuple is either persistent or linear reused
   execute_add_node_persistent0(node, state.get_tuple(r), state.preds[r],
                                state);
}

static inline void execute_add_thread_persistent(db::node* node, pcounter& pc,
                                                 state& state) {
   const reg_num r(pcounter_reg(pc + instr_size));

   // tuple is either persistent or linear reused
   execute_add_thread_persistent0(node, state.get_tuple(r), state.preds[r],
                                  state);
}

static inline void execute_run_action(pcounter& pc, state& state) {
   const reg_num r(pcounter_reg(pc + instr_size));
   execute_run_action0(state.get_tuple(r), state.preds[r], state);
}

static inline void execute_enqueue_linear(pcounter& pc, state& state) {
   const reg_num r(pcounter_reg(pc + instr_size));

   execute_enqueue_linear0(state.get_tuple(r), state.preds[r], state);
}

static inline void execute_send(db::node* node, const pcounter& pc,
                                state& state) {
   const reg_num msg(send_msg(pc));
   const reg_num dest(send_dest(pc));
   const node_val dest_val(state.get_node(dest));
   predicate* pred(state.preds[msg]);
   tuple* tuple(state.get_tuple(msg));

   execute_send0(node, dest_val, tuple, pred, state);
}

static inline void execute_thread_send(const pcounter& pc, state& state) {
   const reg_num msg(send_msg(pc));
   const reg_num dest(send_dest(pc));
   const thread_val dest_val(state.get_thread(dest));
   predicate* pred(state.preds[msg]);
   sched::thread* th((sched::thread*)dest_val);
   tuple* tuple(state.get_tuple(msg));

   execute_thread_send0(th, tuple, pred, state);
}

static inline void execute_send_delay(db::node* node, const pcounter& pc,
                                      state& state) {
   const reg_num msg(send_delay_msg(pc));
   const reg_num dest(send_delay_dest(pc));
   const node_val dest_val(state.get_node(dest));
   predicate* pred(state.preds[msg]);
   tuple* tuple(state.get_tuple(msg));

#ifdef CORE_STATISTICS
   state.stat.stat_predicate_proven[pred->get_id()]++;
#endif

   if (msg == dest) {
#ifdef DEBUG_SENDS
      cout << "\t";
      tuple->print(cout, pred);
      cout << " -> self " << state.node->get_id() << endl;
#endif
      state.sched->new_work_delay(node, node, tuple, pred, state.direction,
                                  state.depth, send_delay_time(pc));
   } else {
#ifdef DEBUG_SENDS
      cout << "\t";
      tuple->print(cout, pred);
      cout << " -> " << dest_val << endl;
#endif
#ifdef USE_REAL_NODES
      db::node* node((db::node*)dest_val);
#else
      db::node* node(All->DATABASE->find_node((node::node_id)dest_val));
#endif
      state.sched->new_work_delay(node, node, tuple, pred, state.direction,
                                  state.depth, send_delay_time(pc));
   }
}

static inline void execute_not(pcounter& pc, state& state) {
   const reg_num op(not_op(pc));
   const reg_num dest(not_dest(pc));

   state.set_bool(dest, !state.get_bool(op));
}

#if 0
static inline bool
do_match(predicate *pred, const tuple *tuple, const field_num& field, const instr_val& val,
   pcounter &pc, const state& state)
{
   if(val_is_reg(val)) {
      const reg_num reg(val_reg(val));
      
      switch(pred->get_field_type(field)->get_type()) {
         case FIELD_INT: return tuple->get_int(field) == state.get_int(reg);
         case FIELD_FLOAT: return tuple->get_float(field) == state.get_float(reg);
         case FIELD_NODE: return tuple->get_node(field) == state.get_node(reg);
         default: throw vm_exec_error("matching with non-primitive types in registers is unsupported");
      }
   } else if(val_is_field(val)) {
      const vm::tuple *tuple2(state.get_tuple(val_field_reg(pc)));
      const field_num field2(val_field_num(pc));
      
      pcounter_move_field(&pc);
      
      switch(pred->get_field_type(field)->get_type()) {
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
#endif

static bool do_rec_match(match_field m, tuple_field x, type* t) {
   switch (t->get_type()) {
      case FIELD_INT:
         if (FIELD_INT(m.field) != FIELD_INT(x)) return false;
         break;
      case FIELD_FLOAT:
         if (FIELD_FLOAT(m.field) != FIELD_FLOAT(x)) return false;
         break;
      case FIELD_NODE:
         if (FIELD_NODE(m.field) != FIELD_NODE(x)) return false;
         break;
      case FIELD_LIST:
         if (FIELD_PTR(m.field) == 0) {
            if (!runtime::cons::is_null(FIELD_CONS(x))) return false;
         } else if (FIELD_PTR(m.field) == 1) {
            if (runtime::cons::is_null(FIELD_CONS(x))) return false;
         } else {
            runtime::cons* ls(FIELD_CONS(x));
            if (runtime::cons::is_null(ls)) return false;
            list_match* lm(FIELD_LIST_MATCH(m.field));
            list_type* lt((list_type*)t);
            if (lm->head.exact &&
                !do_rec_match(lm->head, ls->get_head(), lt->get_subtype()))
               return false;
            tuple_field tail;
            SET_FIELD_CONS(tail, ls->get_tail());
            if (lm->tail.exact && !do_rec_match(lm->tail, tail, lt))
               return false;
         }
         break;
      default:
         throw vm_exec_error("can't match this argument");
   }
   return true;
}

static inline bool do_matches(match* m, const tuple* tuple, predicate* pred) {
   if (!m || !m->any_exact) return true;

   for (size_t i(0); i < pred->num_fields(); ++i) {
      if (m->has_match(i)) {
         type* t = pred->get_field_type(i);
         match_field mf(m->get_match(i));
         if (!do_rec_match(mf, tuple->get_field(i), t)) return false;
      }
   }
   return true;
}

static void build_match_element(instr_val val, match* m, type* t,
                                match_field* mf, pcounter& pc, state& state,
                                size_t& count) {
   switch (t->get_type()) {
      case FIELD_INT:
         if (val_is_reg(val)) {
            const reg_num reg(val_reg(val));
            const int_val i(state.get_int(reg));
            m->match_int(mf, i);
            variable_match_template vmt;
            vmt.match = mf;
            vmt.type = MATCH_REG;
            vmt.reg = reg;
            m->add_variable_match(vmt, count);
            count++;
         } else if (val_is_field(val)) {
            const reg_num reg(val_field_reg(pc));
            const tuple* tuple(state.get_tuple(reg));
            const field_num field(val_field_num(pc));

            pcounter_move_field(&pc);
            const int_val i(tuple->get_int(field));
            m->match_int(mf, i);
            variable_match_template vmt;
            vmt.match = mf;
            vmt.type = MATCH_FIELD;
            vmt.reg = reg;
            vmt.field = field;
            m->add_variable_match(vmt, count);
            ++count;
         } else if (val_is_int(val)) {
            const int_val i(pcounter_int(pc));
            m->match_int(mf, i);
            pcounter_move_int(&pc);
         } else
            throw vm_exec_error("cannot use value for matching int");
         break;
      case FIELD_FLOAT:
         if (val_is_field(val)) {
            const reg_num reg(val_field_reg(pc));
            const tuple* tuple(state.get_tuple(reg));
            const field_num field(val_field_num(pc));

            pcounter_move_field(&pc);
            const float_val f(tuple->get_float(field));
            m->match_float(mf, f);
            variable_match_template vmt;
            vmt.match = mf;
            vmt.type = MATCH_FIELD;
            vmt.reg = reg;
            vmt.field = field;
            m->add_variable_match(vmt, count);
            ++count;
         } else if (val_is_float(val)) {
            const float_val f(pcounter_float(pc));
            m->match_float(mf, f);
            pcounter_move_float(&pc);
         } else
            throw vm_exec_error("cannot use value for matching float");
         break;
      case FIELD_NODE:
         if (val_is_reg(val)) {
            const reg_num reg(val_reg(val));
            const node_val n(state.get_node(reg));
            m->match_node(mf, n);
            variable_match_template vmt;
            vmt.match = mf;
            vmt.type = MATCH_REG;
            vmt.reg = reg;
            m->add_variable_match(vmt, count);
            ++count;
         } else if (val_is_field(val)) {
            const reg_num reg(val_field_reg(pc));
            const tuple* tuple(state.get_tuple(reg));
            const field_num field(val_field_num(pc));

            pcounter_move_field(&pc);
            const node_val n(tuple->get_node(field));
            m->match_node(mf, n);
            variable_match_template vmt;
            vmt.match = mf;
            vmt.type = MATCH_FIELD;
            vmt.reg = reg;
            vmt.field = field;
            m->add_variable_match(vmt, count);
            ++count;
         } else if(val_is_host(val)) {
            m->match_node(mf, (vm::node_val)state.node);
            variable_match_template vmt;
            vmt.match = mf;
            vmt.type = MATCH_HOST;
            m->add_variable_match(vmt, count);
            ++count;
         } else if (val_is_node(val)) {
            const node_val n(pcounter_node(pc));
            m->match_node(mf, n);
            pcounter_move_node(&pc);
         } else
            throw vm_exec_error("cannot use value for matching node");
         break;
      case FIELD_LIST:
         if (val_is_any(val)) {
            mf->exact = false;
            // do nothing
         } else if (val_is_nil(val)) {
            m->match_nil(mf);
         } else if (val_is_non_nil(val)) {
            m->match_non_nil(mf);
         } else if (val_is_list(val)) {
            list_type* lt((list_type*)t);
            list_match* lm(mem::allocator<list_match>().allocate(1));
            lm->init(lt);
            const instr_val head(val_get(pc, 0));
            pcounter_move_byte(&pc);
            if (!val_is_any(head))
               build_match_element(head, m, lt->get_subtype(), &(lm->head), pc,
                                   state, count);
            const instr_val tail(val_get(pc, 0));
            pcounter_move_byte(&pc);
            if (!val_is_any(tail))
               build_match_element(tail, m, t, &(lm->tail), pc, state, count);
            m->match_list(mf, lm, t);
         } else
            throw vm_exec_error("invalid field type for ITERATE/FIELD_LIST");
         break;
      default:
         throw vm_exec_error("invalid field type for ITERATE");
   }
}

static inline void build_match_object(match* m, pcounter pc,
                                      const predicate* pred, state& state,
                                      size_t matches) {
   type* t;
   size_t count(0);

   for (size_t i(0); i < matches; ++i) {
      const field_num field(iter_match_field(pc));
      const instr_val val(iter_match_val(pc));

      pcounter_move_match(&pc);

      t = pred->get_field_type(field);
      build_match_element(val, m, t, m->get_update_match(field), pc, state,
                          count);
   }
}

static size_t count_var_match_element(instr_val val, type* t, pcounter& pc) {
   switch (t->get_type()) {
      case FIELD_INT:
         if (val_is_reg(val))
            return 1;
         else if (val_is_field(val)) {
            pcounter_move_field(&pc);
            return 1;
         } else if (val_is_int(val))
            pcounter_move_int(&pc);
         else
            throw vm_exec_error("cannot use value for matching int");
         break;
      case FIELD_FLOAT: {
         if (val_is_reg(val))
            return 1;
         else if (val_is_field(val)) {
            pcounter_move_field(&pc);
            return 1;
         } else if (val_is_float(val))
            pcounter_move_float(&pc);
         else
            throw vm_exec_error("cannot use value for matching float");
      } break;
      case FIELD_NODE:
         if (val_is_reg(val))
            return 1;
         else if (val_is_field(val)) {
            pcounter_move_field(&pc);
            return 1;
         } else if(val_is_host(val))
            return 1;
         else if (val_is_node(val))
            pcounter_move_node(&pc);
         else
            throw vm_exec_error("cannot use value for matching node");
         break;
      case FIELD_LIST:
         if (val_is_any(val)) {
         } else if (val_is_nil(val)) {
         } else if (val_is_non_nil(val)) {
         } else if (val_is_list(val)) {
            list_type* lt((list_type*)t);
            const instr_val head(val_get(pc, 0));
            pcounter_move_byte(&pc);
            size_t ret(0);
            if (!val_is_any(head))
               ret += count_var_match_element(head, lt->get_subtype(), pc);
            const instr_val tail(val_get(pc, 0));
            pcounter_move_byte(&pc);
            if (!val_is_any(tail)) ret += count_var_match_element(tail, t, pc);
            return ret;
         } else
            throw vm_exec_error("invalid field type for ITERATE/FIELD_LIST");
         break;
      default:
         throw vm_exec_error("invalid field type for ITERATE");
   }
   return 0;
}

static inline size_t count_variable_match_elements(pcounter pc,
                                                   const predicate* pred,
                                                   const size_t matches) {
   type* t;
   size_t ret(0);

   for (size_t i(0); i < matches; ++i) {
      const field_num field(iter_match_field(pc));
      const instr_val val(iter_match_val(pc));

      pcounter_move_match(&pc);

      t = pred->get_field_type(field);
      ret += count_var_match_element(val, t, pc);
   }
   return ret;
}

#include "vm/order.cpp"

static inline match* retrieve_match_object(state& state, pcounter pc,
                                           const predicate* pred,
                                           const size_t base) {
   match* mobj(nullptr);
   const size_t matches = iter_matches_size(pc, base);

   if (matches > 0) {
      utils::byte* m((utils::byte*)iter_match_object(pc));

      if (m == nullptr) {
         const size_t var_size(
             count_variable_match_elements(pc + base, pred, matches));
         const size_t mem(match::mem_size(pred, var_size));
         if (mem > MATCH_OBJECT_SIZE)
            throw vm_exec_error("Match object is too large.");
         m = (utils::byte*)mem::allocator<utils::byte>().allocate(
             MATCH_OBJECT_SIZE * All->NUM_THREADS);
         for (size_t i(0); i < All->NUM_THREADS; ++i) {
            match* obj((match*)(m + i * MATCH_OBJECT_SIZE));
            obj->init(pred, var_size);
            build_match_object(obj, pc + base, pred, state, matches);
         }
         ptr_val* pos(iter_match_object_pos(pc));
         ptr_val old(cmpxchg(pos, (ptr_val) nullptr, (ptr_val)m));

         if (old != (ptr_val) nullptr) {
            // somebody already set this.
            mem::allocator<utils::byte>().deallocate(
                m, MATCH_OBJECT_SIZE * All->NUM_THREADS);
            m = (utils::byte*)old;
         } else
            state.allocated_match_objects.insert(m);
      }
      m += state.sched->get_id() * MATCH_OBJECT_SIZE;
      mobj = (match*)m;

      if (!iter_constant_match(pc)) {
         for (size_t i(0); i < mobj->var_size; ++i) {
            variable_match_template& tmp(mobj->get_variable_match(i));
            switch(tmp.type) {
               case MATCH_FIELD:
                  {
                     tuple* tpl(state.get_tuple(tmp.reg));
                     tmp.match->field = tpl->get_field(tmp.field);
                     break;
                  }
               case MATCH_REG:
                  // we want to retrieve a register value
                  tmp.match->field = state.get_reg(tmp.reg);
                  break;
               case MATCH_HOST:
                  SET_FIELD_NODE(tmp.match->field, state.node);
                  break;
            }
         }
      }
   }

#ifdef DYNAMIC_INDEXING
   if (mobj && state.sched && state.sched->get_id() == 0 && mobj->size() > 0 &&
       pred->is_linear_pred()) {
      // increment statistics for matching
      const int start(pred->get_argument_position());
      const int end(start + pred->num_fields() - 1);
      for (int i(0); i < (int)mobj->size(); ++i) {
         if (mobj->has_match(i)) {
            state.match_counter->increment_count(start + i, start, end);
         }
      }
   }
#endif
   return mobj;
}

// iterate macros
#define PUSH_CURRENT_STATE(TUPLE, TUPLE_LEAF, TUPLE_QUEUE, NEW_DEPTH) \
   state.is_linear = this_is_linear || state.is_linear;               \
   state.depth =                                                      \
       !pred->is_cycle_pred() ? old_depth : max((NEW_DEPTH) + 1, old_depth)
#define POP_STATE()                 \
   state.is_linear = old_is_linear; \
   state.depth = old_depth
#define TO_FINISH(ret) ((ret) == RETURN_LINEAR || (ret) == RETURN_DERIVED)

static inline return_type execute_pers_iter(const reg_num reg, match* m,
                                            const pcounter first, state& state,
                                            predicate* pred, db::node* node) {
   const depth_t old_depth(state.depth);
   const bool old_is_linear(state.is_linear);
   const bool this_is_linear(false);

   if(pred->is_compact_pred()) {
      db::array *a(node->pers_store.get_array(pred));
      for(auto it(a->begin(pred)), end(a->end(pred)); it != end; ++it) {
         vm::tuple *match_tuple(*it);
         if(!do_matches(m, match_tuple, pred))
            continue;
         PUSH_CURRENT_STATE(match_tuple, nullptr, nullptr, (vm::depth_t)0);
#ifdef DEBUG_ITERS
         cout << "\titerate ";
         match_tuple->print(cout, pred);
         cout << "\n";
#endif

         return_type ret = execute(first, state, reg, match_tuple, pred);

         POP_STATE();

         if (ret == RETURN_LINEAR) return ret;
         if (ret == RETURN_DERIVED && state.is_linear) return RETURN_DERIVED;
      }
      return RETURN_NO_RETURN;
   }

   for (auto tuples_it(
            node->pers_store.match_predicate(pred->get_persistent_id(), m));
        !tuples_it.end(); ++tuples_it) {
      tuple_trie_leaf* tuple_leaf(*tuples_it);

      // we get the tuple later since the previous leaf may have been deleted
      tuple* match_tuple(tuple_leaf->get_underlying_tuple());
      assert(match_tuple != nullptr);

#ifdef TRIE_MATCHING_ASSERT
      assert(do_matches(m, match_tuple, pred));
#endif

      PUSH_CURRENT_STATE(match_tuple, tuple_leaf, nullptr,
                         tuple_leaf->get_min_depth());
#ifdef DEBUG_ITERS
      cout << "\titerate ";
      match_tuple->print(cout, pred);
      cout << "\n";
#endif

      return_type ret = execute(first, state, reg, match_tuple, pred);

      POP_STATE();

      if (ret == RETURN_LINEAR) return ret;
      if (ret == RETURN_DERIVED && state.is_linear) return RETURN_DERIVED;
   }

   return RETURN_NO_RETURN;
}

static inline return_type execute_olinear_iter(const reg_num reg, match* m,
                                               const pcounter pc,
                                               const pcounter first,
                                               state& state, predicate* pred,
                                               db::node* node) {
   const depth_t old_depth(state.depth);
   const bool old_is_linear(state.is_linear);
   const bool this_is_linear(true);
   const utils::byte options(iter_options(pc));
   const utils::byte options_arguments(iter_options_argument(pc));

   vector_iter tpls;

   utils::intrusive_list<vm::tuple>* local_tuples(
       node->linear.get_linked_list(pred->get_linear_id()));
#ifdef CORE_STATISTICS
   execution_time::scope s(state.stat.ts_search_time_predicate[pred->get_id()]);
#endif
   for (utils::intrusive_list<vm::tuple>::iterator it(local_tuples->begin()),
        end(local_tuples->end());
        it != end; ++it) {
      tuple* tpl(*it);
      if (!state.tuple_is_used(tpl, reg) && do_matches(m, tpl, pred)) {
         iter_object obj;
         obj.tpl = tpl;
         obj.iterator = it;
         obj.ls = local_tuples;
         tpls.push_back(obj);
      }
   }

   if (iter_options_random(options))
      utils::shuffle_vector(tpls, state.randgen);
   else if (iter_options_min(options)) {
      const field_num field(iter_options_min_arg(options_arguments));

      sort(tpls.begin(), tpls.end(), tuple_sorter(field, pred));
   } else
      throw vm_exec_error("do not know how to use this selector");

   for (auto it(tpls.begin()); it != tpls.end();) {
      iter_object p(*it);
      auto ls(p.ls);
      return_type ret;

      tuple* match_tuple(p.tpl);

      if (state.tuple_is_used(match_tuple, reg))
         goto next_tuple;

      PUSH_CURRENT_STATE(match_tuple, nullptr, match_tuple, (vm::depth_t)0);

      ret = execute(first, state, reg, match_tuple, pred);

      POP_STATE();

      if (TO_FINISH(ret)) {
         utils::intrusive_list<vm::tuple>::iterator it(p.iterator);
         ls->erase(it);
         vm::tuple::destroy(match_tuple, pred, &(node->alloc), state.gc_nodes);
         if (ret == RETURN_LINEAR) return RETURN_LINEAR;
         if (ret == RETURN_DERIVED && old_is_linear) return RETURN_DERIVED;
      }
   next_tuple:
      // removed item from the list because it is no longer needed
      it = tpls.erase(it);
   }

   return RETURN_NO_RETURN;
}

static inline return_type execute_orlinear_iter(const reg_num reg, match* m,
                                                const pcounter pc,
                                                const pcounter first,
                                                state& state, predicate* pred,
                                                db::node* node) {
   const depth_t old_depth(state.depth);
   const bool old_is_linear(state.is_linear);
   const bool this_is_linear(false);
   const utils::byte options(iter_options(pc));
   const utils::byte options_arguments(iter_options_argument(pc));

   vector_iter tpls;

   utils::intrusive_list<vm::tuple>* local_tuples(
       node->linear.get_linked_list(pred->get_linear_id()));
#ifdef CORE_STATISTICS
   execution_time::scope s(state.stat.ts_search_time_predicate[pred->get_id()]);
#endif
   for (utils::intrusive_list<vm::tuple>::iterator it(local_tuples->begin()),
        end(local_tuples->end());
        it != end; ++it) {
      tuple* tpl(*it);
      if (!state.tuple_is_used(tpl, reg) && do_matches(m, tpl, pred)) {
         iter_object obj;
         obj.tpl = tpl;
         obj.iterator = it;
         tpls.push_back(obj);
      }
   }

   if (iter_options_random(options))
      utils::shuffle_vector(tpls, state.randgen);
   else if (iter_options_min(options)) {
      const field_num field(iter_options_min_arg(options_arguments));

      sort(tpls.begin(), tpls.end(), tuple_sorter(field, pred));
   } else
      throw vm_exec_error("do not know how to use this selector");

   for (auto it(tpls.begin()); it != tpls.end();) {
      iter_object p(*it);
      return_type ret;

      tuple* match_tuple(p.tpl);

      if (state.tuple_is_used(match_tuple, reg))
         goto next_tuple;

      PUSH_CURRENT_STATE(match_tuple, nullptr, match_tuple, (vm::depth_t)0);

      ret = execute(first, state, reg, match_tuple, pred);

      POP_STATE();

      if (ret == RETURN_LINEAR) return RETURN_LINEAR;
      if (ret == RETURN_DERIVED && old_is_linear) return RETURN_DERIVED;
   next_tuple:
      // removed item from the list because it is no longer needed
      it = tpls.erase(it);
   }

   return RETURN_NO_RETURN;
}

static inline return_type execute_opers_iter(const reg_num reg, match* m,
                                             const pcounter pc,
                                             const pcounter first, state& state,
                                             predicate* pred, db::node* node) {
   const depth_t old_depth(state.depth);
   const bool old_is_linear(state.is_linear);
   const bool this_is_linear(false);
   const utils::byte options(iter_options(pc));
   const utils::byte options_arguments(iter_options_argument(pc));

   vector_leaves leaves;

   for (auto tuples_it(
            node->pers_store.match_predicate(pred->get_persistent_id(), m));
        !tuples_it.end(); ++tuples_it) {
      tuple_trie_leaf* tuple_leaf(*tuples_it);
#ifdef TRIE_MATCHING_ASSERT
      assert(do_matches(m, tuple_leaf->get_underlying_tuple(), pred));
#endif
      leaves.push_back(tuple_leaf);
   }

   if (iter_options_random(options))
      utils::shuffle_vector(leaves, state.randgen);
   else if (iter_options_min(options)) {
      const field_num field(iter_options_min_arg(options_arguments));

      sort(leaves.begin(), leaves.end(), tuple_leaf_sorter(field, pred));
   } else
      throw vm_exec_error("do not know how to use this selector");

   for (auto it(leaves.begin()); it != leaves.end();) {
      tuple_trie_leaf* tuple_leaf(*it);

      tuple* match_tuple(tuple_leaf->get_underlying_tuple());
      assert(match_tuple != nullptr);

      PUSH_CURRENT_STATE(match_tuple, tuple_leaf, nullptr,
                         tuple_leaf->get_min_depth());

      return_type ret(execute(first, state, reg, match_tuple, pred));

      POP_STATE();

      if (ret == RETURN_LINEAR) return RETURN_LINEAR;
      if (ret == RETURN_DERIVED && old_is_linear) return RETURN_DERIVED;
      it = leaves.erase(it);
   }

   return RETURN_NO_RETURN;
}

static inline return_type execute_linear_iter_list(
    db::node *node, const reg_num reg, match* m, const pcounter first, state& state,
    predicate* pred, vm::tuple_list* local_tuples,
    hash_table* tbl = nullptr) {
   if (local_tuples == nullptr) return RETURN_NO_RETURN;

   const bool old_is_linear(state.is_linear);
   const bool this_is_linear(true);
   const depth_t old_depth(state.depth);

   for (auto it(local_tuples->begin()),
        end(local_tuples->end());
        it != end;) {
      tuple* match_tuple(*it);

      if(state.tuple_is_used(match_tuple, reg)) {
         it++;
         continue;
      }

      {
#ifdef CORE_STATISTICS
         execution_time::scope s2(
             state.stat.ts_search_time_predicate[pred->get_id()]);
#endif
         if (!do_matches(m, match_tuple, pred)) {
            it++;
            continue;
         }
      }

      PUSH_CURRENT_STATE(match_tuple, nullptr, match_tuple, (vm::depth_t)0);
#ifdef DEBUG_ITERS
      cout << "\titerate ";
      match_tuple->print(cout, pred);
      cout << "\n";
#endif

      const return_type ret(execute(first, state, reg, match_tuple, pred));

      POP_STATE();

      bool next_iter = true;

      if (TO_FINISH(ret)) {
         if(state.updated_map.get_bit(reg)) {
            state.updated_map.unset_bit(reg);
            if (reg > 0) {
               // if this is the first iterate, we do not need to send this to
               // the generate list
               if (tbl)
                  it = tbl->erase_from_list(db::hash_table::cast_list(local_tuples), it, &(node->alloc));
               else
                  it = local_tuples->erase(it);
               state.add_generated(match_tuple, pred);
               next_iter = false;
            } else {
               if (tbl) {
                  // may need to re hash tuple
                  // it is not a problem if the tuple gets in the same bucket
                  // (will appear at the end of the list)
                  // it = local_tuples->erase(it);
                  // add the tuple to the back of the bucket
                  // that way, we do not see it again if it's added to the same
                  // bucket
                  // tbl->insert(match_tuple);
                  // next_iter = false;
               }
            }
         } else {
            if (tbl)
               it = tbl->erase_from_list(db::hash_table::cast_list(local_tuples), it, &(node->alloc));
            else
               it = local_tuples->erase(it);
            vm::tuple::destroy(match_tuple, pred, &(node->alloc), state.gc_nodes);
            if (tbl) {
               if (tbl->empty()) {
                  // local_tuples is now invalid!
                  state.matcher->empty_predicate(pred->get_id());
               }
            } else {
               if (local_tuples->empty())
                  state.matcher->empty_predicate(pred->get_id());
            }
            next_iter = false;
         }
      }

      if (ret == RETURN_LINEAR) return RETURN_LINEAR;
      if (old_is_linear && ret == RETURN_DERIVED) return RETURN_DERIVED;
      if (next_iter)
         it++;
   }
   return RETURN_NO_RETURN;
}

static inline return_type execute_linear_iter(const reg_num reg, match* m,
                                              const pcounter first,
                                              state& state, predicate* pred,
                                              db::node* node) {
   if (node->linear.stored_as_hash_table(pred)) {
      const field_num hashed(pred->get_hashed_field());
      hash_table* table(node->linear.get_hash_table(pred->get_linear_id()));

      if (table == nullptr) return RETURN_NO_RETURN;

      if (m && m->has_match(hashed)) {
         const match_field mf(m->get_match(hashed));
         vm::tuple_list *local_tuples(db::hash_table::underlying_list(table->lookup_list(mf.field)));
         return_type ret(execute_linear_iter_list(node, reg, m, first, state, pred,
                                                  local_tuples, table));
         return ret;
      } else {
         // go through hash table
         for (hash_table::iterator it(table->begin()); !it.end(); ++it) {
            vm::tuple_list* local_tuples(db::hash_table::underlying_list(*it));
            return_type ret(execute_linear_iter_list(node, reg, m, first, state, pred,
                                                     local_tuples, table));
            if (ret != RETURN_NO_RETURN) return ret;
         }
         return RETURN_NO_RETURN;
      }
   } else {
      utils::intrusive_list<vm::tuple>* local_tuples(
          node->linear.get_linked_list(pred->get_linear_id()));
      return execute_linear_iter_list(node, reg, m, first, state, pred, local_tuples);
   }
}

static inline return_type execute_rlinear_iter_list(
    const reg_num reg, match* m, const pcounter first, state& state,
    predicate* pred, utils::intrusive_list<vm::tuple>* local_tuples) {
   const bool old_is_linear(state.is_linear);
   const bool this_is_linear(false);
   const depth_t old_depth(state.depth);

   if(!local_tuples)
      return RETURN_NO_RETURN;

   for(auto it(local_tuples->begin()), e(local_tuples->end()); it != e; ++it) {
      vm::tuple *match_tuple(*it);
      if(state.tuple_is_used(match_tuple, reg))
         continue;

      {
#ifdef CORE_STATISTICS
         execution_time::scope s2(
             state.stat.ts_search_time_predicate[pred->get_id()]);
#endif
         if (!do_matches(m, match_tuple, pred)) continue;
      }

      PUSH_CURRENT_STATE(match_tuple, nullptr, match_tuple, (vm::depth_t)0);
#ifdef DEBUG_ITERS
      cout << "\titerate ";
      match_tuple->print(cout, pred);
      cout << "\n";
#endif

      const return_type ret(execute(first, state, reg, match_tuple, pred));

      POP_STATE();

      if (ret == RETURN_LINEAR) return RETURN_LINEAR;
      if (old_is_linear && ret == RETURN_DERIVED) return RETURN_DERIVED;
   }

   return RETURN_NO_RETURN;
}

static inline return_type execute_rlinear_iter(const reg_num reg, match* m,
                                               const pcounter first,
                                               state& state, predicate* pred,
                                               db::node* node) {
   if (node->linear.stored_as_hash_table(pred)) {
      const field_num hashed(pred->get_hashed_field());
      hash_table* table(node->linear.get_hash_table(pred->get_linear_id()));

      if (m && m->has_match(hashed)) {
         const match_field mf(m->get_match(hashed));
         auto ls(table->lookup_list(mf.field));
         vm::tuple_list* local_tuples(db::hash_table::underlying_list(ls));
         return execute_rlinear_iter_list(reg, m, first, state, pred,
                                          local_tuples);
      } else {
         // go through hash table
         for (hash_table::iterator it(table->begin()); !it.end(); ++it) {
            vm::tuple_list* local_tuples(db::hash_table::underlying_list(*it));
            return_type ret(execute_rlinear_iter_list(reg, m, first, state,
                                                      pred, local_tuples));
            if (ret != RETURN_NO_RETURN) return ret;
         }
         return RETURN_NO_RETURN;
      }
   } else {
      utils::intrusive_list<vm::tuple>* local_tuples(
          node->linear.get_linked_list(pred->get_linear_id()));
      return execute_rlinear_iter_list(reg, m, first, state, pred,
                                       local_tuples);
   }
}

static inline void execute_testnil(pcounter pc, state& state) {
   const reg_num op(test_nil_op(pc));
   const reg_num dest(test_nil_dest(pc));

   runtime::cons* x(state.get_cons(op));

   state.set_bool(dest, runtime::cons::is_null(x));
}

static inline void execute_float(pcounter& pc, state& state) {
   const reg_num src(pcounter_reg(pc + instr_size));
   const reg_num dst(pcounter_reg(pc + instr_size + reg_val_size));

   state.set_float(dst, static_cast<float_val>(state.get_int(src)));
}

static inline void execute_fabs(pcounter& pc, state& state) {
   const reg_num src(pcounter_reg(pc + instr_size));
   const reg_num dst(pcounter_reg(pc + instr_size + reg_val_size));

   state.set_float(dst, fabs(state.get_float(src)));
}

static inline pcounter execute_select(db::node* node, pcounter pc,
                                      state& state) {
   (void)state;
   if (node->get_id() > All->DATABASE->static_max_id())
      return pc + select_size(pc);

   const pcounter hash_start(select_hash_start(pc));
   const code_size_t hashed(select_hash(hash_start, node->get_id()));

   if (hashed == 0)  // no specific code
      return pc + select_size(pc);
   return select_hash_code(hash_start, select_hash_size(pc), hashed);
}

static inline void execute_remove(pcounter pc, state& state) {
   const reg_num reg(pcounter_reg(pc + instr_size));
   vm::tuple* tpl(state.get_tuple(reg));
   vm::predicate* pred(state.preds[reg]);

#ifdef CORE_STATISTICS
   state.stat.stat_predicate_success[pred->get_id()]++;
#endif

#ifdef DEBUG_REMOVE
   cout << "\tdelete ";
   tpl->print(cout, pred);
   cout << endl;
#endif
   assert(tpl);

   if (state.direction == POSITIVE_DERIVATION) {
      if (pred->is_reused_pred()) {
         auto stpl(full_tuple::remove_new(tpl, pred, state.depth));
         if (pred->is_thread_pred())
            state.add_thread_persistent_fact(stpl);
         else
            state.add_node_persistent_fact(stpl);
      }
      state.linear_facts_consumed++;
#ifdef INSTRUMENTATION
      state.instr_facts_consumed++;
#endif
      LOG_DELETED_FACT();
   }
}

static inline void execute_update(pcounter pc, state& state) {
   const reg_num reg(pcounter_reg(pc + instr_size));
   vm::predicate* pred(state.preds[reg]);

   state.updated_map.set_bit(reg);
#ifdef DEBUG_SENDS
   vm::tuple *tpl(state.get_tuple(reg));
   cout << "\tupdate ";
   tpl->print(cout, pred);
   cout << endl;
#endif
   state.matcher->register_predicate_update(pred->get_id());
}

static inline void set_call_return(const reg_num reg, const utils::byte typ,
                                   const bool gc, const tuple_field ret,
                                   external_function* f, state& state) {
   (void)f;
   type* ret_type(theProgram->get_type(typ));
   assert(ret_type);
   switch (ret_type->get_type()) {
      case FIELD_BOOL:
         state.set_bool(reg, FIELD_BOOL(ret));
         break;
      case FIELD_INT:
         state.set_int(reg, FIELD_INT(ret));
         break;
      case FIELD_FLOAT:
         state.set_float(reg, FIELD_FLOAT(ret));
         break;
      case FIELD_NODE:
         state.set_node(reg, FIELD_NODE(ret));
         break;
      case FIELD_THREAD:
         state.set_thread(reg, FIELD_THREAD(ret));
         break;
      case FIELD_STRING: {
         rstring::ptr s(FIELD_STRING(ret));

         state.set_string(reg, s);
         if (gc) state.add_string(s);

         break;
      }
      case FIELD_LIST: {
         cons* l(FIELD_CONS(ret));

         state.set_cons(reg, l);
         if (gc && !cons::is_null(l)) state.add_cons(l, (list_type*)ret_type);
         break;
      }
      case FIELD_STRUCT: {
         struct1* s(FIELD_STRUCT(ret));

         state.set_struct(reg, s);
         if (gc) state.add_struct(s, (struct_type*)ret_type);
         break;
      }
      case FIELD_ARRAY: {
         runtime::array* a(FIELD_ARRAY(ret));
         state.set_array(reg, a);
         if (gc) state.add_array(a, ((array_type*)ret_type)->get_base());
         break;
      }
      case FIELD_SET: {
         runtime::set* a(FIELD_SET(ret));
         state.set_set(reg, a);
         if (gc) state.add_set(a, ((set_type*)ret_type)->get_base());
         break;
      }
      case FIELD_ANY:
         state.set_reg(reg, ret);
         break;
      default:
         throw vm_exec_error("invalid return type in call (set_call_return)");
   }
}

static inline void execute_call0(pcounter& pc, state& state) {
   const external_function_id id(call_extern_id(pc));
   external_function* f(lookup_external_function(id));

   assert(f->get_num_args() == 0);

   argument ret = f->get_fun_ptr()();
   set_call_return(call_dest(pc), call_type(pc), call_gc(pc), ret, f, state);
}

static inline void execute_call1(pcounter& pc, state& state) {
   const external_function_id id(call_extern_id(pc));
   external_function* f(lookup_external_function(id));

   assert(f->get_num_args() == 1);

   argument ret = ((external_function_ptr1)f->get_fun_ptr())(
       state.get_reg(pcounter_reg(pc + call_size)));
   set_call_return(call_dest(pc), call_type(pc), call_gc(pc), ret, f, state);
}

static inline void execute_call2(pcounter& pc, state& state) {
   const external_function_id id(call_extern_id(pc));
   external_function* f(lookup_external_function(id));

   assert(f->get_num_args() == 2);

   argument ret = ((external_function_ptr2)f->get_fun_ptr())(
       state.get_reg(pcounter_reg(pc + call_size)),
       state.get_reg(pcounter_reg(pc + call_size + reg_val_size)));
   set_call_return(call_dest(pc), call_type(pc), call_gc(pc), ret, f, state);
}

static inline void execute_call3(pcounter& pc, state& state) {
   const external_function_id id(call_extern_id(pc));
   external_function* f(lookup_external_function(id));

   assert(f->get_num_args() == 3);

   argument ret = ((external_function_ptr3)f->get_fun_ptr())(
       state.get_reg(pcounter_reg(pc + call_size)),
       state.get_reg(pcounter_reg(pc + call_size + reg_val_size)),
       state.get_reg(pcounter_reg(pc + call_size + 2 * reg_val_size)));
   set_call_return(call_dest(pc), call_type(pc), call_gc(pc), ret, f, state);
}

static inline argument do_call(external_function* f, argument* args) {
   switch (f->get_num_args()) {
      case 0:
         return f->get_fun_ptr()();
      case 1:
         return ((external_function_ptr1)f->get_fun_ptr())(args[0]);
      case 2:
         return ((external_function_ptr2)f->get_fun_ptr())(args[0], args[1]);
      case 3:
         return ((external_function_ptr3)f->get_fun_ptr())(args[0], args[1],
                                                           args[2]);
      case 4:
         return ((external_function_ptr4)f->get_fun_ptr())(args[0], args[1],
                                                           args[2], args[3]);
      case 5:
         return ((external_function_ptr5)f->get_fun_ptr())(
             args[0], args[1], args[2], args[3], args[4]);
      case 6:
         return ((external_function_ptr6)f->get_fun_ptr())(
             args[0], args[1], args[2], args[3], args[4], args[5]);
      default:
         throw vm_exec_error(
             "vm does not support external functions with more than 3 "
             "arguments");
   }

   // undefined
   argument ret;
   return ret;
}

static inline void execute_call(pcounter& pc, state& state) {
   const external_function_id id(call_extern_id(pc));
   external_function* f(lookup_external_function(id));
   const size_t num_args(call_num_args(pc));
   argument args[num_args];

   pcounter m(pc + CALL_BASE);
   for (size_t i(0); i < num_args; ++i) {
      args[i] = state.get_reg(pcounter_reg(m));
      m += reg_val_size;
   }

   assert(num_args == f->get_num_args());

   set_call_return(call_dest(pc), call_type(pc), call_gc(pc), do_call(f, args),
                   f, state);
}

static inline void execute_calle(pcounter pc, state& state) {
   const external_function_id id(calle_extern_id(pc) +
                                 first_custom_external_function());
   const size_t num_args(calle_num_args(pc));
   external_function* f(lookup_external_function(id));
   argument args[num_args];

   pcounter m(pc + CALLE_BASE);
   for (size_t i(0); i < num_args; ++i) {
      args[i] = state.get_reg(pcounter_reg(m));
      m += reg_val_size;
   }

   assert(num_args == f->get_num_args());

   set_call_return(calle_dest(pc), calle_type(pc), calle_gc(pc),
                   do_call(f, args), f, state);
}

static inline void execute_set_priority(pcounter& pc, state& state) {
   const reg_num prio_reg(pcounter_reg(pc + instr_size));
   const reg_num node_reg(pcounter_reg(pc + instr_size + reg_val_size));
   const priority_t prio(state.get_float(prio_reg));
   const node_val node(state.get_node(node_reg));

   if (!scheduling_mechanism) return;

   execute_set_priority0(node, prio, state);
}

static inline void execute_set_priority_here(db::node* node, pcounter& pc,
                                             state& state) {
   const reg_num prio_reg(pcounter_reg(pc + instr_size));
   const float_val prio(state.get_float(prio_reg));

   state.sched->set_node_priority(node, prio);
}

static inline void execute_add_priority(pcounter& pc, state& state) {
   const reg_num prio_reg(pcounter_reg(pc + instr_size));
   const reg_num node_reg(pcounter_reg(pc + instr_size + reg_val_size));
   const float_val prio(state.get_float(prio_reg));
   const node_val node(state.get_node(node_reg));

#ifdef USE_REAL_NODES
   state.sched->add_node_priority((db::node*)node, prio);
#else
   state.sched->add_node_priority(All->DATABASE->find_node(node), prio);
#endif
}

static inline void execute_add_priority_here(db::node* node, pcounter& pc,
                                             state& state) {
   const reg_num prio_reg(pcounter_reg(pc + instr_size));
   const float_val prio(state.get_float(prio_reg));

   state.sched->add_node_priority(node, prio);
}

static inline void execute_rem_priority(pcounter& pc, state& state) {
   const reg_num node_reg(pcounter_reg(pc + instr_size));
   const node_val node(state.get_node(node_reg));

#ifdef USE_REAL_NODES
   state.sched->remove_node_priority((db::node*)node);
#else
   state.sched->remove_node_priority(All->DATABASE->find_node(node));
#endif
}

static inline void execute_rem_priority_here(db::node* node, pcounter& pc,
                                             state& state) {
   (void)pc;
   state.sched->remove_node_priority(node);
}

static inline void execute_schedule_next(pcounter& pc, state& state) {
   const reg_num node_reg(pcounter_reg(pc + instr_size));
   const node_val node(state.get_node(node_reg));
   db::node* nodep;
#ifdef USE_REAL_NODES
   nodep = (db::node*)node;
#else
   nodep = All->DATABASE->find_node(node);
#endif

   state.sched->schedule_next(nodep);
}

static inline void execute_set_defprio_here(db::node* node, pcounter& pc,
                                            state& state) {
   const reg_num prio_reg(pcounter_reg(pc + instr_size));
   const float_val prio(state.get_float(prio_reg));

   state.sched->set_default_node_priority(node, prio);
}

static inline void execute_set_defprio(pcounter& pc, state& state) {
   const reg_num prio_reg(pcounter_reg(pc + instr_size));
   const reg_num node_reg(pcounter_reg(pc + instr_size + reg_val_size));
   const float_val prio(state.get_float(prio_reg));
   const node_val node(state.get_node(node_reg));

#ifdef USE_REAL_NODES
   state.sched->set_default_node_priority((db::node*)node, prio);
#else
   state.sched->set_default_node_priority(All->DATABASE->find_node(node), prio);
#endif
}

static inline void execute_set_static_here(db::node* node, pcounter& pc,
                                           state& state) {
   (void)pc;
   state.sched->set_node_static(node);
}

static inline void execute_set_static(pcounter& pc, state& state) {
   const reg_num node_reg(pcounter_reg(pc + instr_size));
   const node_val node(state.get_node(node_reg));

#ifdef USE_REAL_NODES
   state.sched->set_node_static((db::node*)node);
#else
   state.sched->set_node_static(All->DATABASE->find_node(node));
#endif
}

static inline void execute_set_moving_here(db::node* node, pcounter& pc,
                                           state& state) {
   (void)pc;
   state.sched->set_node_moving(node);
}

static inline void execute_set_moving(pcounter& pc, state& state) {
   const reg_num node_reg(pcounter_reg(pc + instr_size));
   const node_val node(state.get_node(node_reg));

#ifdef USE_REAL_NODES
   state.sched->set_node_moving((db::node*)node);
#else
   state.sched->set_node_moving(All->DATABASE->find_node(node));
#endif
}

static inline void execute_set_affinity_here(db::node* node, pcounter& pc,
                                             state& state) {
   const reg_num target_reg(pcounter_reg(pc + instr_size));
   const node_val target(state.get_node(target_reg));

#ifdef USE_REAL_NODES
   state.sched->set_node_affinity(node, (db::node*)target);
#else
   state.sched->set_node_affinity(node, All->DATABASE->find_node(target));
#endif
}

static inline void execute_set_affinity(pcounter& pc, state& state) {
   const reg_num target_reg(pcounter_reg(pc + instr_size));
   const node_val target(state.get_node(target_reg));
   const reg_num node_reg(pcounter_reg(pc + instr_size + reg_val_size));
   const node_val node(state.get_node(node_reg));

#ifdef USE_REAL_NODES
   state.sched->set_node_affinity((db::node*)node, (db::node*)target);
#else
   state.sched->set_node_affinity(All->DATABASE->find_node(node),
                                  All->DATABASE->find_node(target));
#endif
}

static inline void execute_cpu_id(pcounter& pc, state& state) {
   const reg_num node_reg(pcounter_reg(pc + instr_size));
   const reg_num dest_reg(pcounter_reg(pc + instr_size + reg_val_size));
   const node_val nodeval(state.get_node(node_reg));
#ifdef USE_REAL_NODES
   db::node* node((db::node*)nodeval);
#else
   db::node* node(All->DATABASE->find_node(nodeval));
#endif

   sched::thread* owner(node->get_owner());
   state.set_thread(dest_reg, (vm::thread_val)owner);
}

static inline void execute_cpu_static(pcounter& pc, state& state) {
   const reg_num node_reg(pcounter_reg(pc + instr_size));
   const reg_num dest_reg(pcounter_reg(pc + instr_size + reg_val_size));
   const node_val nodeval(state.get_node(node_reg));
#ifdef USE_REAL_NODES
   db::node* node((db::node*)nodeval);
#else
   db::node* node(All->DATABASE->find_node(nodeval));
#endif
   sched::thread* owner(static_cast<sched::thread*>(node->get_owner()));

   state.set_int(dest_reg, owner->num_static_nodes());
}

static inline void execute_is_static(pcounter& pc, state& state) {
   const reg_num node_reg(pcounter_reg(pc + instr_size));
   const reg_num dest_reg(pcounter_reg(pc + instr_size + reg_val_size));
   const node_val nodeval(state.get_node(node_reg));
#ifdef USE_REAL_NODES
   db::node* node((db::node*)nodeval);
#else
   db::node* node(All->DATABASE->find_node(nodeval));
#endif

   state.set_int(dest_reg, node->is_static() ? 1 : 0);
}

static inline void execute_is_moving(pcounter& pc, state& state) {
   const reg_num node_reg(pcounter_reg(pc + instr_size));
   const reg_num dest_reg(pcounter_reg(pc + instr_size + reg_val_size));
   const node_val nodeval(state.get_node(node_reg));
#ifdef USE_REAL_NODES
   db::node* node((db::node*)nodeval);
#else
   db::node* node(All->DATABASE->find_node(nodeval));
#endif
   state.set_int(dest_reg, node->is_static() ? 0 : 1);
}

static inline void execute_facts_proved(pcounter& pc, state& state) {
   const reg_num node_reg(pcounter_reg(pc + instr_size));
   const reg_num dest_reg(pcounter_reg(pc + instr_size + reg_val_size));
   const node_val nodeval(state.get_node(node_reg));
#ifdef USE_REAL_NODES
   db::node* node((db::node*)nodeval);
#else
   db::node* node(All->DATABASE->find_node(nodeval));
#endif
   (void)node;
   assert(node == state.node);
   state.set_int(dest_reg, state.linear_facts_generated +
                               state.persistent_facts_generated);
}

static inline void execute_facts_consumed(pcounter& pc, state& state) {
   const reg_num node_reg(pcounter_reg(pc + instr_size));
   const reg_num dest_reg(pcounter_reg(pc + instr_size + reg_val_size));
   const node_val nodeval(state.get_node(node_reg));
#ifdef USE_REAL_NODES
   db::node* node((db::node*)nodeval);
#else
   db::node* node(All->DATABASE->find_node(nodeval));
#endif
   (void)node;
   assert(node == state.node);
   state.set_int(dest_reg, state.linear_facts_consumed);
}

static inline void execute_set_cpu_here(db::node* node, pcounter& pc,
                                        state& state) {
   const reg_num cpu_reg(pcounter_reg(pc + instr_size));

   state.sched->set_node_cpu(node, (sched::thread*)state.get_thread(cpu_reg));
}

static inline void execute_node_priority(pcounter& pc, state& state) {
   const reg_num node_reg(pcounter_reg(pc + instr_size));
   const reg_num dest_reg(pcounter_reg(pc + instr_size + reg_val_size));
   const node_val nodeval(state.get_node(node_reg));
#ifdef USE_REAL_NODES
   db::node* node((db::node*)nodeval);
#else
   db::node* node(All->DATABASE->find_node(nodeval));
#endif

   state.set_float(dest_reg, node->get_priority());
}

static inline void execute_mark_rule(const pcounter& pc, state& state) {
   const size_t rule_id(rule_get_id(pc));
   rule *r(theProgram->get_rule(rule_id));
   if(state.matcher->rules[rule_id] == r->num_predicates())
      state.matcher->rule_queue.set_bit(rule_id);
}

static inline void execute_literal_cons(pcounter pc, state& state) {
   const utils::byte tid(literal_cons_type(pc));
   const reg_num dest(literal_cons_dest(pc));
   pc += LITERAL_CONS_BASE;
   vm::tuple_field data(axiom_read_data(pc, vm::theProgram->get_type(tid)));
   state.set_cons(dest, FIELD_CONS(data));
}

static inline void execute_node_type(pcounter pc, state& state) {
   const reg_num dest(node_type_reg(pc));
   pc += NODE_TYPE_BASE;
   state.set_int(dest, (int_val)pc[state.node->get_id()]);
}

static inline void execute_rule(const pcounter& pc, state& state) {
   const size_t rule_id(rule_get_id(pc));

   state.current_rule = rule_id;

#ifdef CORE_STATISTICS
   if (state.stat.stat_rules_activated == 0 && state.stat.stat_inside_rule) {
      state.stat.stat_rules_failed++;
   }
   state.stat.stat_inside_rule = true;
   state.stat.stat_rules_activated = 0;
#endif
}

static inline void execute_rule_done(const pcounter& pc, state& state) {
   (void)pc;
   (void)state;

#ifdef CORE_STATISTICS
   state.stat.stat_rules_ok++;
   state.stat.stat_rules_activated++;
#endif

#if 0
   const string rule_str(state.PROGRAM->get_rule_string(state.current_rule));

   cout << "Rule applied " << rule_str << endl;
#endif
}

static inline void execute_new_node(const pcounter& pc, state& state) {
   const reg_num reg(new_node_reg(pc));
   node* new_node(state.sched->create_node());

#ifdef USE_REAL_NODES
   state.set_node(reg, (node_val)new_node);
#else
   state.set_node(reg, new_node->get_id());
#endif
}

static inline void execute_new_axioms(db::node* node, pcounter pc,
                                      state& state) {
   const pcounter end(pc + new_axioms_jump(pc));
   pc += NEW_AXIOMS_BASE;

   add_new_axioms(state, node, pc, end);
}

static inline bool perform_remote_update(
    utils::intrusive_list<vm::tuple>* local_tuples, predicate* pred_target,
    const size_t common, tuple_field* regs, state& state) {
   for (auto match_tuple : *local_tuples) {
      for (size_t i(0); i < common; ++i) {
         vm::type* t(pred_target->get_field_type(i));
         match_field m = {true, t, regs[i]};
         if (!do_rec_match(m, match_tuple->get_field(i), t)) goto continue2;
      }

      // update tuple.
      for (size_t i(common); i < pred_target->num_fields(); ++i) {
         const tuple_field old(match_tuple->get_field(i));
         vm::type* ftype(pred_target->get_field_type(i));
         tuple_set_field(match_tuple, ftype, i, regs[i]);
         if (ftype->is_reference())
            runtime::do_decrement_runtime(old, ftype, state.gc_nodes);
      }
      return true;

   continue2:
      continue;
   }
   return false;
}

static inline void execute_remote_update(pcounter& pc, state& state) {
   const reg_num dest(remote_update_dest(pc));
   const predicate_id edit(remote_update_edit(pc));
   const predicate_id target(remote_update_target(pc));
   predicate* pred_edit(theProgram->get_predicate(edit));
   predicate* pred_target(theProgram->get_predicate(target));
   const size_t common(remote_update_common(pc));
   const size_t nregs(remote_update_nregs(pc));
   tuple_field regs[nregs];
   for (size_t i(0); i < nregs; ++i)
      regs[i] = state.get_reg(
          pcounter_reg(pc + REMOTE_UPDATE_BASE + i * reg_val_size));
   vm::node_val n0(state.get_node(dest));
   db::node* n;
#ifdef USE_REAL_NODES
   n = (db::node*)n0;
#else
   abort();
#endif

   bool updated{false};
   LOCK_STACK(internal_lock_data);
   if (n->database_lock.try_lock1(LOCK_STACK_USE(internal_lock_data))) {
      if (n->linear.stored_as_hash_table(pred_target)) {
         const field_num h(pred_target->get_hashed_field());
         hash_table* table(
             n->linear.get_hash_table(pred_target->get_linear_id()));

         if (table) {
            if (h < common)
               updated =
                   perform_remote_update(db::hash_table::underlying_list(table->lookup_list(regs[h])),
                                         pred_target, common, regs, state);
            else {
               for (auto it(table->begin()); !it.end(); ++it) {
                  if ((updated = perform_remote_update(db::hash_table::underlying_list(*it),
                              pred_target, common, regs, state)))
                     break;
               }
            }
         }
      } else
         updated = perform_remote_update(
             n->linear.get_linked_list(pred_target->get_linear_id()),
             pred_target, common, regs, state);
      MUTEX_UNLOCK(n->database_lock, internal_lock_data);
   }
   if (updated) return;
   tuple* tuple(vm::tuple::create(pred_edit, &(n->alloc)));
   for (size_t i(0); i < pred_edit->num_fields(); ++i)
      tuple_set_field(tuple, pred_edit->get_field_type(i), i, regs[i]);
   execute_send0(state.node, n0, tuple, pred_edit, state);
}

static inline void execute_make_structr(pcounter& pc, state& state) {
   struct_type* st((struct_type*)theProgram->get_type(make_structr_type(pc)));
   const reg_num to(pcounter_reg(pc + instr_size + type_size));

   struct1* s(struct1::create(st));

   for (size_t i(0); i < st->get_size(); ++i)
      s->set_data(i, *state.stack.get_stack_at(i), st);
   state.stack.pop(st->get_size());

   state.set_struct(to, s);
   state.add_struct(s, st);
}

static inline void execute_make_structf(pcounter& pc, state& state) {
   predicate* pred(state.preds[val_field_reg(pc + instr_size)]);
   tuple* tuple(get_tuple_field(state, pc + instr_size));
   const field_num field(val_field_num(pc + instr_size));
   struct_type* st((struct_type*)pred->get_field_type(field));

   struct1* s(struct1::create(st));

   for (size_t i(0); i < st->get_size(); ++i)
      s->set_data(i, *state.stack.get_stack_at(i), st);
   state.stack.pop(st->get_size());

   tuple->set_struct(field, s);
}

static inline void execute_struct_valrr(pcounter& pc, state& state) {
   const size_t idx(struct_val_idx(pc));
   const reg_num src(pcounter_reg(pc + instr_size + count_size));
   const reg_num dst(pcounter_reg(pc + instr_size + count_size + reg_val_size));
   state.set_reg(dst, state.get_struct(src)->get_data(idx));
}

static inline void execute_struct_valfr(pcounter& pc, state& state) {
   const size_t idx(struct_val_idx(pc));
   tuple* tuple(get_tuple_field(state, pc + instr_size + count_size));
   const field_num field(val_field_num(pc + instr_size + count_size));
   const reg_num dst(pcounter_reg(pc + instr_size + count_size + field_size));
   struct1* s(tuple->get_struct(field));
   state.set_reg(dst, s->get_data(idx));
}

static inline void execute_struct_valrf(pcounter& pc, state& state) {
   const size_t idx(struct_val_idx(pc));
   const reg_num src(pcounter_reg(pc + instr_size + count_size));
   tuple* tuple(
       get_tuple_field(state, pc + instr_size + count_size + reg_val_size));
   const field_num field(
       val_field_num(pc + instr_size + count_size + reg_val_size));
   struct1* s(state.get_struct(src));
   tuple->set_field(field, s->get_data(idx));
}

static inline void execute_struct_valrfr(pcounter& pc, state& state) {
   const size_t idx(struct_val_idx(pc));
   const reg_num src(pcounter_reg(pc + instr_size + count_size));
   tuple* tuple(
       get_tuple_field(state, pc + instr_size + count_size + reg_val_size));
   const field_num field(
       val_field_num(pc + instr_size + count_size + reg_val_size));
   struct1* s(state.get_struct(src));
   tuple->set_field(field, s->get_data(idx));
   do_increment_runtime(tuple->get_field(field), FIELD_STRUCT);
}

static inline void execute_struct_valff(pcounter& pc, state& state) {
   tuple* src(get_tuple_field(state, pc + instr_size + count_size));
   const field_num field_src(val_field_num(pc + instr_size + count_size));
   tuple* dst(
       get_tuple_field(state, pc + instr_size + count_size + field_size));
   const field_num field_dst(
       val_field_num(pc + instr_size + count_size + field_size));

   dst->set_field(field_dst, src->get_field(field_src));
}

static inline void execute_struct_valffr(pcounter& pc, state& state) {
   tuple* src(get_tuple_field(state, pc + instr_size + count_size));
   const field_num field_src(val_field_num(pc + instr_size + count_size));
   tuple* dst(
       get_tuple_field(state, pc + instr_size + count_size + field_size));
   const field_num field_dst(
       val_field_num(pc + instr_size + count_size + field_size));

   dst->set_field(field_dst, src->get_field(field_src));

   do_increment_runtime(dst->get_field(field_dst), FIELD_STRUCT);
}

static inline void execute_mvintfield(pcounter pc, state& state) {
   tuple* tuple(get_tuple_field(state, pc + instr_size + int_size));

   tuple->set_int(val_field_num(pc + instr_size + int_size),
                  pcounter_int(pc + instr_size));
}

static inline void execute_mvintreg(pcounter pc, state& state) {
   state.set_int(pcounter_reg(pc + instr_size + int_size),
                 pcounter_int(pc + instr_size));
}

static inline void execute_mvtypereg(pcounter pc, state& state) {
   vm::type* ty(theProgram->get_type(pcounter_int(pc + instr_size)));
   state.set_ptr(pcounter_reg(pc + instr_size + int_size), (ptr_val)ty);
}

static inline void execute_mvfieldfield(pcounter pc, state& state) {
   tuple* tuple_from(get_tuple_field(state, pc + instr_size));
   tuple* tuple_to(get_tuple_field(state, pc + instr_size + field_size));
   const field_num from(val_field_num(pc + instr_size));
   const field_num to(val_field_num(pc + instr_size + field_size));
   tuple_to->set_field(to, tuple_from->get_field(from));
}

static inline void execute_mvfieldfieldr(pcounter pc, state& state) {
   tuple* tuple_from(get_tuple_field(state, pc + instr_size));
   tuple* tuple_to(get_tuple_field(state, pc + instr_size + field_size));
   predicate* pred_to(state.preds[val_field_reg(pc + instr_size + field_size)]);
   const field_num from(val_field_num(pc + instr_size));
   const field_num to(val_field_num(pc + instr_size + field_size));

   tuple_to->set_field_ref(to, tuple_from->get_field(from), pred_to,
                           state.gc_nodes);
}

static inline void execute_mvfieldreg(pcounter pc, state& state) {
   tuple* tuple_from(get_tuple_field(state, pc + instr_size));
   const field_num from(val_field_num(pc + instr_size));

   state.set_reg(pcounter_reg(pc + instr_size + field_size),
                 tuple_from->get_field(from));
}

static inline void execute_mvptrreg(pcounter pc, state& state) {
   const ptr_val p(pcounter_ptr(pc + instr_size));
   const reg_num reg(pcounter_reg(pc + instr_size + ptr_size));

   state.set_ptr(reg, p);
}

static inline void execute_mvnilfield(pcounter& pc, state& state) {
   tuple* tuple(get_tuple_field(state, pc + instr_size));

   tuple->set_nil(val_field_num(pc + instr_size));
}

static inline void execute_mvnilreg(pcounter& pc, state& state) {
   state.set_nil(pcounter_reg(pc + instr_size));
}

static inline void execute_mvregfield(pcounter& pc, state& state) {
   const reg_num reg(pcounter_reg(pc + instr_size));
   tuple* tuple(get_tuple_field(state, pc + instr_size + reg_val_size));
   const field_num field(val_field_num(pc + instr_size + reg_val_size));
   tuple->set_field(field, state.get_reg(reg));
}

static inline void execute_mvregfieldr(pcounter& pc, state& state) {
   const reg_num reg(pcounter_reg(pc + instr_size));
   predicate* pred(state.preds[val_field_reg(pc + instr_size + reg_val_size)]);
   tuple* tuple(get_tuple_field(state, pc + instr_size + reg_val_size));
   const field_num field(val_field_num(pc + instr_size + reg_val_size));

   tuple->set_field_ref(field, state.get_reg(reg), pred, state.gc_nodes);

   assert(reference_type(pred->get_field_type(field)->get_type()));
}

static inline void execute_mvhostfield(db::node* node, pcounter& pc,
                                       state& state) {
   tuple* tuple(get_tuple_field(state, pc + instr_size));
   const field_num field(val_field_num(pc + instr_size));

#ifdef USE_REAL_NODES
   tuple->set_node(field, (node_val)node);
#else
   tuple->set_node(field, state.node->get_id());
#endif
}

static inline void execute_mvthreadidreg(pcounter& pc, state& state) {
   const reg_num reg(pcounter_reg(pc + instr_size));

   state.set_thread(reg, (thread_val)state.sched);
}

static inline void execute_mvthreadidfield(pcounter& pc, state& state) {
   tuple* tpl(get_tuple_field(state, pc + instr_size));
   const field_num field(val_field_num(pc + instr_size));

   tpl->set_thread(field, (thread_val)state.sched);
}

static inline void execute_mvregconst(pcounter& pc, state& state) {
   const const_id id(pcounter_const_id(pc + instr_size + reg_val_size));
   All->set_const(id, state.get_reg(pcounter_reg(pc + instr_size)));
   const field_type ftype(theProgram->get_const_type(id)->get_type());
   increment_runtime_data(All->get_const(id), ftype);
}

static inline void execute_mvconstfield(pcounter& pc, state& state) {
   const const_id id(pcounter_const_id(pc + instr_size));
   const field_num field(val_field_num(pc + instr_size + const_id_size));
   tuple* tuple(get_tuple_field(state, pc + instr_size + const_id_size));

   tuple->set_field(field, All->get_const(id));
}

static inline void execute_mvconstfieldr(pcounter& pc, state& state) {
   const const_id id(pcounter_const_id(pc + instr_size));
   const field_num field(val_field_num(pc + instr_size + const_id_size));
   predicate* pred(state.preds[val_field_reg(pc + instr_size + const_id_size)]);
   tuple* tuple(get_tuple_field(state, pc + instr_size + const_id_size));

   tuple->set_field_ref(field, All->get_const(id), pred, state.gc_nodes);
}

static inline void execute_mvconstreg(pcounter& pc, state& state) {
   const const_id id(pcounter_const_id(pc + instr_size));
   const reg_num reg(pcounter_reg(pc + instr_size + const_id_size));

   state.set_reg(reg, All->get_const(id));
}

static inline void execute_mvintstack(pcounter& pc, state& state) {
   const int_val i(pcounter_int(pc + instr_size));
   const offset_num off(pcounter_stack(pc + instr_size + int_size));
   state.stack.get_stack_at(off)->int_field = i;
}

static inline void execute_mvargreg(pcounter& pc, state& state) {
   const argument_id arg(pcounter_argument_id(pc + instr_size));
   const reg_num reg(pcounter_reg(pc + instr_size + argument_size));

   state.set_string(reg, All->get_argument(arg));
}

static inline void execute_mvfloatstack(pcounter& pc, state& state) {
   const float_val f(pcounter_float(pc + instr_size));
   const offset_num off(pcounter_stack(pc + instr_size + float_size));
   state.stack.get_stack_at(off)->float_field = f;
}

static inline void execute_mvaddrfield(pcounter& pc, state& state) {
   const node_val n(pcounter_node(pc + instr_size));
   const field_num field(val_field_num(pc + instr_size + node_size));
   tuple* tuple(get_tuple_field(state, pc + instr_size + node_size));

   tuple->set_node(field, n);
}

static inline void execute_mvfloatfield(pcounter& pc, state& state) {
   const float_val f(pcounter_float(pc + instr_size));
   const field_num field(val_field_num(pc + instr_size + float_size));
   tuple* tuple(get_tuple_field(state, pc + instr_size + float_size));

   tuple->set_float(field, f);
}

static inline void execute_mvfloatreg(pcounter& pc, state& state) {
   const float_val f(pcounter_float(pc + instr_size));
   const reg_num reg(pcounter_reg(pc + instr_size + float_size));

   state.set_float(reg, f);
}

static inline void execute_mvintconst(pcounter& pc) {
   const int_val i(pcounter_int(pc + instr_size));
   const const_id id(pcounter_const_id(pc + instr_size + int_size));

   All->set_const_int(id, i);
}

static inline void execute_mvworldfield(pcounter& pc, state& state) {
   const field_num field(val_field_num(pc + instr_size));
   tuple* tuple(get_tuple_field(state, pc + instr_size));

   tuple->set_int(field, All->DATABASE->nodes_total);
}

static inline void execute_mvworldreg(pcounter& pc, state& state) {
   const reg_num reg(pcounter_reg(pc + instr_size));

   state.set_int(reg, All->DATABASE->nodes_total);
}

static inline void execute_mvcpusreg(pcounter& pc, state& state) {
   const reg_num reg(pcounter_reg(pc + instr_size));

   state.set_int(reg, All->NUM_THREADS);
}

static inline pcounter execute_mvstackpcounter(pcounter& pc, state& state) {
   const offset_num off(pcounter_stack(pc + instr_size));

   return (pcounter)FIELD_PCOUNTER(*(state.stack.get_stack_at(off))) +
          CALLF_BASE;
}

static inline void execute_mvpcounterstack(pcounter& pc, state& state) {
   const offset_num off(pcounter_stack(pc + instr_size));

   SET_FIELD_PTR(*(state.stack.get_stack_at(off)), pc + MVPCOUNTERSTACK_BASE);
}

static inline void execute_mvstackreg(pcounter& pc, state& state) {
   const offset_num off(pcounter_stack(pc + instr_size));
   const reg_num reg(pcounter_reg(pc + instr_size + stack_val_size));

   state.set_reg(reg, *state.stack.get_stack_at(off));
}

static inline void execute_mvstackfield(pcounter& pc, state& state) {
   const offset_num off(pcounter_stack(pc + instr_size));
   const field_num field(val_field_num(pc + instr_size + reg_val_size));
   tuple* tuple(get_tuple_field(state, pc + instr_size + reg_val_size));

   tuple->set_field(field, *state.stack.get_stack_at(off));
}

static inline void execute_mvregstack(pcounter& pc, state& state) {
   const reg_num reg(pcounter_reg(pc + instr_size));
   const offset_num off(pcounter_stack(pc + instr_size + reg_val_size));

   *(state.stack.get_stack_at(off)) = state.get_reg(reg);
}

static inline void execute_mvaddrreg(pcounter& pc, state& state) {
   const node_val n(pcounter_node(pc + instr_size));
   const reg_num reg(pcounter_reg(pc + instr_size + node_size));

   state.set_node(reg, n);
}

static inline void execute_mvhostreg(db::node* node, pcounter& pc,
                                     state& state) {
   const reg_num reg(pcounter_reg(pc + instr_size));
#ifdef USE_REAL_NODES
   state.set_node(reg, (node_val)node);
#else
   state.set_node(reg, node->get_id());
#endif
}

static inline void execute_mvregreg(pcounter& pc, state& state) {
   state.copy_reg(pcounter_reg(pc + instr_size),
                  pcounter_reg(pc + instr_size + reg_val_size));
}

#define DO_OPERATION(SET_FUNCTION, GET_FUNCTION, OP)                    \
   const reg_num op1(pcounter_reg(pc + instr_size));                    \
   const reg_num op2(pcounter_reg(pc + instr_size + reg_val_size));     \
   const reg_num dst(pcounter_reg(pc + instr_size + 2 * reg_val_size)); \
   state.SET_FUNCTION(dst, state.GET_FUNCTION(op1) OP state.GET_FUNCTION(op2))
#define BOOL_OPERATION(GET_FUNCTION, OP) \
   DO_OPERATION(set_bool, GET_FUNCTION, OP)

static inline void execute_addrnotequal(pcounter& pc, state& state) {
   BOOL_OPERATION(get_node, != );
}

static inline void execute_addrequal(pcounter& pc, state& state) {
   BOOL_OPERATION(get_node, == );
}

static inline void execute_intminus(pcounter& pc, state& state) {
   DO_OPERATION(set_int, get_int, -);
}

static inline void execute_intequal(pcounter& pc, state& state) {
   DO_OPERATION(set_int, get_int, == );
}

static inline void execute_intnotequal(pcounter& pc, state& state) {
   DO_OPERATION(set_int, get_int, != );
}

static inline void execute_intplus(pcounter& pc, state& state) {
   DO_OPERATION(set_int, get_int, +);
}

static inline void execute_intlesser(pcounter& pc, state& state) {
   DO_OPERATION(set_bool, get_int, < );
}

static inline void execute_intgreaterequal(pcounter& pc, state& state) {
   DO_OPERATION(set_bool, get_int, >= );
}

static inline void execute_boolor(pcounter& pc, state& state) {
   DO_OPERATION(set_bool, get_bool, || );
}

static inline void execute_intlesserequal(pcounter& pc, state& state) {
   DO_OPERATION(set_bool, get_int, <= );
}

static inline void execute_intgreater(pcounter& pc, state& state) {
   DO_OPERATION(set_bool, get_int, > );
}

static inline void execute_intmul(pcounter& pc, state& state) {
   DO_OPERATION(set_int, get_int, *);
}

static inline void execute_intdiv(pcounter& pc, state& state) {
   DO_OPERATION(set_int, get_int, / );
}

static inline void execute_intmod(pcounter& pc, state& state) {
   DO_OPERATION(set_int, get_int, % );
}

static inline void execute_floatplus(pcounter& pc, state& state) {
   DO_OPERATION(set_float, get_float, +);
}

static inline void execute_floatminus(pcounter& pc, state& state) {
   DO_OPERATION(set_float, get_float, -);
}

static inline void execute_floatmul(pcounter& pc, state& state) {
   DO_OPERATION(set_float, get_float, *);
}

static inline void execute_floatdiv(pcounter& pc, state& state) {
   DO_OPERATION(set_float, get_float, / );
}

static inline void execute_floatequal(pcounter& pc, state& state) {
   DO_OPERATION(set_bool, get_float, == );
}

static inline void execute_floatnotequal(pcounter& pc, state& state) {
   DO_OPERATION(set_bool, get_float, != );
}

static inline void execute_floatlesser(pcounter& pc, state& state) {
   DO_OPERATION(set_bool, get_float, < );
}

static inline void execute_floatlesserequal(pcounter& pc, state& state) {
   DO_OPERATION(set_bool, get_float, <= );
}

static inline void execute_floatgreater(pcounter& pc, state& state) {
   DO_OPERATION(set_bool, get_float, > );
}

static inline void execute_floatgreaterequal(pcounter& pc, state& state) {
   DO_OPERATION(set_bool, get_float, >= );
}

static inline void execute_boolequal(pcounter& pc, state& state) {
   DO_OPERATION(set_bool, get_bool, == );
}

static inline void execute_boolnotequal(pcounter& pc, state& state) {
   DO_OPERATION(set_bool, get_bool, != );
}

static inline void execute_headrr(pcounter& pc, state& state) {
   const reg_num ls(pcounter_reg(pc + instr_size));
   const reg_num dst(pcounter_reg(pc + instr_size + reg_val_size));

   runtime::cons* l(state.get_cons(ls));

   state.set_reg(dst, l->get_head());
}

static inline void execute_headfr(pcounter& pc, state& state) {
   tuple* tuple(get_tuple_field(state, pc + instr_size));
   const field_num field(val_field_num(pc + instr_size));
   runtime::cons* l(tuple->get_cons(field));
   const reg_num dst(pcounter_reg(pc + instr_size + field_size));

   state.set_reg(dst, l->get_head());
}

static inline void execute_headff(pcounter& pc, state& state) {
   tuple* tsrc(get_tuple_field(state, pc + instr_size));
   const field_num src(val_field_num(pc + instr_size));
   tuple* tdst(get_tuple_field(state, pc + instr_size + field_size));
   const field_num dst(val_field_num(pc + instr_size + field_size));

   runtime::cons* l(tsrc->get_cons(src));

   tdst->set_field(dst, l->get_head());
}

static inline void execute_headffr(pcounter& pc, state& state) {
   tuple* tsrc(get_tuple_field(state, pc + instr_size));
   const field_num src(val_field_num(pc + instr_size));
   tuple* tdst(get_tuple_field(state, pc + instr_size + field_size));
   const field_num dst(val_field_num(pc + instr_size + field_size));

   runtime::cons* l(tsrc->get_cons(src));

   tdst->set_field(dst, l->get_head());
   do_increment_runtime(tdst->get_field(dst), FIELD_LIST);
}

static inline void execute_headrf(pcounter& pc, state& state) {
   const reg_num reg(pcounter_reg(pc + instr_size));
   tuple* tdst(get_tuple_field(state, pc + instr_size + reg_val_size));
   const field_num dst(val_field_num(pc + instr_size + reg_val_size));

   runtime::cons* l(state.get_cons(reg));

   tdst->set_field(dst, l->get_head());
}

static inline void execute_headrfr(pcounter& pc, state& state) {
   const reg_num reg(pcounter_reg(pc + instr_size));
   tuple* tdst(get_tuple_field(state, pc + instr_size + reg_val_size));
   const field_num dst(val_field_num(pc + instr_size + reg_val_size));

   runtime::cons* l(state.get_cons(reg));

   tdst->set_field(dst, l->get_head());

   do_increment_runtime(tdst->get_field(dst), FIELD_LIST);
}

static inline void execute_tailrr(pcounter& pc, state& state) {
   const reg_num src(pcounter_reg(pc + instr_size));
   const reg_num dst(pcounter_reg(pc + instr_size + reg_val_size));

   runtime::cons* l(state.get_cons(src));

   state.set_cons(dst, l->get_tail());
}

static inline void execute_tailfr(pcounter& pc, state& state) {
   tuple* tuple(get_tuple_field(state, pc + instr_size));
   const field_num field(val_field_num(pc + instr_size));
   const reg_num dst(pcounter_reg(pc + instr_size + field_size));

   state.set_cons(dst, tuple->get_cons(field)->get_tail());
}

static inline void execute_tailff(pcounter& pc, state& state) {
   tuple* tsrc(get_tuple_field(state, pc + instr_size));
   const field_num src(val_field_num(pc + instr_size));
   tuple* tdst(get_tuple_field(state, pc + instr_size + field_size));
   const field_num dst(val_field_num(pc + instr_size + field_size));

   tdst->set_cons(dst, tsrc->get_cons(src)->get_tail());
}

static inline void execute_tailrf(pcounter& pc, state& state) {
   const reg_num src(pcounter_reg(pc + instr_size));
   tuple* tuple(get_tuple_field(state, pc + instr_size + reg_val_size));
   const field_num field(val_field_num(pc + instr_size + reg_val_size));

   tuple->set_cons(field, state.get_cons(src)->get_tail());
}

static inline void execute_consrrr(pcounter& pc, state& state) {
   const reg_num head(pcounter_reg(pc + instr_size + type_size));
   const reg_num tail(pcounter_reg(pc + instr_size + type_size + reg_val_size));
   const reg_num dest(
       pcounter_reg(pc + instr_size + type_size + 2 * reg_val_size));
   const bool_val gc(
       pcounter_bool(pc + instr_size + type_size + 3 * reg_val_size));

   list_type* ltype((list_type*)theProgram->get_type(cons_type(pc)));
   cons* new_list(cons::create(state.get_cons(tail), state.get_reg(head),
                               ltype->get_subtype()));
   if (gc) state.add_cons(new_list, ltype);
   state.set_cons(dest, new_list);
}

static inline void execute_consrff(pcounter& pc, state& state) {
   const reg_num head(pcounter_reg(pc + instr_size));
   predicate* pred(state.preds[val_field_reg(pc + instr_size + reg_val_size)]);
   tuple* tail(get_tuple_field(state, pc + instr_size + reg_val_size));
   const field_num tail_field(val_field_num(pc + instr_size + reg_val_size));
   tuple* dest(
       get_tuple_field(state, pc + instr_size + reg_val_size + field_size));
   const field_num dest_field(
       val_field_num(pc + instr_size + reg_val_size + field_size));

   cons* new_list(cons::create(
       tail->get_cons(tail_field), state.get_reg(head),
       ((list_type*)pred->get_field_type(tail_field))->get_subtype()));
   dest->set_cons(dest_field, new_list);
}

static inline void execute_consfrf(pcounter& pc, state& state) {
   tuple* head(get_tuple_field(state, pc + instr_size));
   const field_num head_field(val_field_num(pc + instr_size));
   const reg_num tail(pcounter_reg(pc + instr_size + field_size));
   predicate* pred(
       state.preds[val_field_reg(pc + instr_size + field_size + reg_val_size)]);
   tuple* dest(
       get_tuple_field(state, pc + instr_size + field_size + reg_val_size));
   const field_num dest_field(
       val_field_num(pc + instr_size + field_size + reg_val_size));

   cons* new_list(cons::create(
       state.get_cons(tail), head->get_field(head_field),
       ((list_type*)pred->get_field_type(dest_field))->get_subtype()));
   dest->set_cons(dest_field, new_list);
}

static inline void execute_consffr(pcounter& pc, state& state) {
   tuple* head(get_tuple_field(state, pc + instr_size));
   const field_num head_field(val_field_num(pc + instr_size));
   predicate* pred(state.preds[val_field_reg(pc + instr_size + field_size)]);
   tuple* tail(get_tuple_field(state, pc + instr_size + field_size));
   const field_num tail_field(val_field_num(pc + instr_size + field_size));
   const reg_num dest(pcounter_reg(pc + instr_size + 2 * field_size));
   const bool_val gc(
       pcounter_bool(pc + instr_size + 2 * field_size + reg_val_size));

   list_type* lt((list_type*)pred->get_field_type(tail_field));
   cons* new_list(cons::create(tail->get_cons(tail_field),
                               head->get_field(head_field), lt->get_subtype()));
   if (gc) state.add_cons(new_list, lt);
   state.set_cons(dest, new_list);
}

static inline void execute_consrrf(pcounter& pc, state& state) {
   const reg_num head(pcounter_reg(pc + instr_size));
   const reg_num tail(pcounter_reg(pc + instr_size + reg_val_size));
   tuple* dest(get_tuple_field(state, pc + instr_size + 2 * reg_val_size));
   predicate* pred(
       state.preds[val_field_reg(pc + instr_size + 2 * reg_val_size)]);
   const field_num field(val_field_num(pc + instr_size + 2 * reg_val_size));

   cons* new_list(
       cons::create(state.get_cons(tail), state.get_reg(head),
                    ((list_type*)pred->get_field_type(field))->get_subtype()));
   dest->set_cons(field, new_list);
}

static inline void execute_consrfr(pcounter& pc, state& state) {
   const reg_num head(pcounter_reg(pc + instr_size));
   tuple* tail(get_tuple_field(state, pc + instr_size + reg_val_size));
   predicate* pred(state.preds[val_field_reg(pc + instr_size + reg_val_size)]);
   const field_num field(val_field_num(pc + instr_size + reg_val_size));
   const reg_num dest(
       pcounter_reg(pc + instr_size + reg_val_size + field_size));
   const bool_val gc(pcounter_bool(pc + instr_size + reg_val_size + field_size +
                                   reg_val_size));

   list_type* lt((list_type*)pred->get_field_type(field));
   cons* new_list(cons::create(tail->get_cons(field), state.get_reg(head),
                               lt->get_subtype()));
   if (gc) state.add_cons(new_list, lt);
   state.set_cons(dest, new_list);
}

static inline void execute_consfrr(pcounter& pc, state& state) {
   tuple* head(get_tuple_field(state, pc + instr_size + type_size));
   const field_num field(val_field_num(pc + instr_size + type_size));
   const reg_num tail(pcounter_reg(pc + instr_size + type_size + field_size));
   const reg_num dest(
       pcounter_reg(pc + instr_size + type_size + field_size + reg_val_size));
   const bool_val gc(pcounter_bool(pc + instr_size + type_size + field_size +
                                   2 * reg_val_size));

   list_type* ltype((list_type*)theProgram->get_type(cons_type(pc)));
   cons* new_list(cons::create(state.get_cons(tail), head->get_field(field),
                               ltype->get_subtype()));
   if (gc) state.add_cons(new_list, ltype);
   state.set_cons(dest, new_list);
}

static inline void execute_consfff(pcounter& pc, state& state) {
   tuple* head(get_tuple_field(state, pc + instr_size));
   const field_num field_head(val_field_num(pc + instr_size));
   tuple* tail(get_tuple_field(state, pc + instr_size + field_size));
   predicate* pred(state.preds[val_field_reg(pc + instr_size + field_size)]);
   const field_num field_tail(val_field_num(pc + instr_size + field_size));
   tuple* dest(get_tuple_field(state, pc + instr_size + 2 * field_size));
   const field_num field_dest(val_field_num(pc + instr_size + 2 * field_size));

   cons* new_list(cons::create(
       tail->get_cons(field_tail), head->get_field(field_head),
       ((list_type*)pred->get_field_type(field_tail))->get_subtype()));
   dest->set_cons(field_dest, new_list);
}

#ifdef COMPUTED_GOTOS
#define CASE(X)
#define JUMP_NEXT() goto* jump_table[fetch(pc)]
#define JUMP(label, jump_offset)             \
   label : {                                 \
      const pcounter npc = pc + jump_offset; \
      void* to_go = (void*)jump_table[fetch(npc)];
#define COMPLEX_JUMP(label) label : {
#define ADVANCE() \
   pc = npc;      \
   goto* to_go;
#define ENDOP() }
#else
#define JUMP_NEXT() goto eval_loop
#define CASE(INSTR) case INSTR:
#define JUMP(label, jump_offset)             \
   {                                         \
      const pcounter npc = pc + jump_offset; \
      assert(npc == advance(pc));
#define COMPLEX_JUMP(label) {
#define ADVANCE() \
   pc = npc;      \
   JUMP_NEXT();
#define ENDOP() }
#endif

static inline return_type execute(pcounter pc, state& state, const reg_num reg,
                                  tuple* tpl, predicate* pred) {
   if (tpl != nullptr) {
      state.set_tuple(reg, tpl);
      if(pred->is_linear_pred())
         state.tuple_regs.set_bit((size_t)reg);
      state.preds[reg] = pred;
#ifdef CORE_STATISTICS
      state.stat.stat_tuples_used++;
      if (tpl->is_linear()) {
         state.stat.stat_predicate_applications[pred->get_id()]++;
      }
#endif
   }

#ifdef COMPUTED_GOTOS
#include "vm/jump_table.hpp"
#endif

#ifdef COMPUTED_GOTOS
   JUMP_NEXT();
#else
   while (true) {
   eval_loop:

#ifdef DEBUG_INSTRS
      instr_print_simple(pc, 0, theProgram, cout);
#endif

#ifdef CORE_STATISTICS
      state.stat.stat_instructions_executed++;
#endif

      switch (fetch(pc)) {
#endif  // !COMPUTED_GOTOS
   CASE(RETURN_INSTR)
   COMPLEX_JUMP(return_instr)
   return RETURN_OK;
   ENDOP()

   CASE(NEXT_INSTR)
   COMPLEX_JUMP(next_instr)
   return RETURN_NEXT;
   ENDOP()

   CASE(RETURN_LINEAR_INSTR)
   COMPLEX_JUMP(return_linear)
   return RETURN_LINEAR;
   ENDOP()

   CASE(RETURN_DERIVED_INSTR)
   COMPLEX_JUMP(return_derived)
   return RETURN_DERIVED;
   ENDOP()

   CASE(RETURN_SELECT_INSTR)
   COMPLEX_JUMP(return_select)
   pc += return_select_jump(pc);
   JUMP_NEXT();
   ENDOP()

   CASE(IF_INSTR)
   JUMP(if_instr, IF_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_if_tests++;
#endif
   if (!state.get_bool(if_reg(pc))) {
#ifdef CORE_STATISTICS
      state.stat.stat_if_failed++;
#endif
      pc += if_jump(pc);
      JUMP_NEXT();
   }
   ADVANCE();
   ENDOP()

   CASE(IF_ELSE_INSTR)
   JUMP(if_else, IF_ELSE_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_if_tests++;
#endif
   if (!state.get_bool(if_reg(pc))) {
#ifdef CORE_STATISTICS
      state.stat.stat_if_failed++;
#endif
      pc += if_else_jump_else(pc);
      JUMP_NEXT();
   }
   ADVANCE();
   ENDOP()

   CASE(JUMP_INSTR)
   COMPLEX_JUMP(jump)
   pc += jump_get(pc, instr_size) + JUMP_BASE;

   JUMP_NEXT();
   ENDOP()

   CASE(END_LINEAR_INSTR)
   COMPLEX_JUMP(end_linear)
   return RETURN_END_LINEAR;
   ENDOP()

   CASE(RESET_LINEAR_INSTR)
   JUMP(reset_linear, RESET_LINEAR_BASE) {
      const bool old_is_linear(state.is_linear);

      state.is_linear = false;

      return_type ret(
          execute(pc + RESET_LINEAR_BASE, state, 0, nullptr, nullptr));

      assert(ret == RETURN_END_LINEAR);
      (void)ret;

      state.is_linear = old_is_linear;

      pc += reset_linear_jump(pc);

      JUMP_NEXT();
   }
   ADVANCE();
   ENDOP()

#define DECIDE_NEXT_ITER_INSTR()                                        \
   if (ret == RETURN_LINEAR) return RETURN_LINEAR;                      \
   if (ret == RETURN_DERIVED && state.is_linear) return RETURN_DERIVED; \
   pc += iter_outer_jump(pc);                                           \
   JUMP_NEXT()

   CASE(PERS_ITER_INSTR)
   COMPLEX_JUMP(pers_iter) {
      predicate* pred(theProgram->get_predicate(iter_predicate(pc)));
      const reg_num reg(iter_reg(pc));
      match* mobj(retrieve_match_object(state, pc, pred, PERS_ITER_BASE));

      const return_type ret(execute_pers_iter(
          reg, mobj, pc + iter_inner_jump(pc), state, pred, state.node));

      DECIDE_NEXT_ITER_INSTR();
   }
   ENDOP()

   CASE(LINEAR_ITER_INSTR)
   COMPLEX_JUMP(linear_iter) {
      predicate* pred(theProgram->get_predicate(iter_predicate(pc)));
      const reg_num reg(iter_reg(pc));
      match* mobj(retrieve_match_object(state, pc, pred, LINEAR_ITER_BASE));

      const return_type ret(execute_linear_iter(
          reg, mobj, pc + iter_inner_jump(pc), state, pred, state.node));

      DECIDE_NEXT_ITER_INSTR();
   }
   ENDOP()

   CASE(TLINEAR_ITER_INSTR)
   COMPLEX_JUMP(tlinear_iter) {
      predicate* pred(theProgram->get_predicate(iter_predicate(pc)));
      const reg_num reg(iter_reg(pc));
      match* mobj(retrieve_match_object(state, pc, pred, TLINEAR_ITER_BASE));

      const return_type ret(
          execute_linear_iter(reg, mobj, pc + iter_inner_jump(pc), state, pred,
                              state.sched->thread_node));

      DECIDE_NEXT_ITER_INSTR();
   }
   ENDOP()

   CASE(TRLINEAR_ITER_INSTR)
   COMPLEX_JUMP(trlinear_iter) {
      predicate* pred(theProgram->get_predicate(iter_predicate(pc)));
      const reg_num reg(iter_reg(pc));
      match *mobj(retrieve_match_object(state, pc, pred, TRLINEAR_ITER_BASE));

      const return_type ret(
            execute_rlinear_iter(reg, mobj, pc + iter_inner_jump(pc), state, pred, state.sched->thread_node));

      DECIDE_NEXT_ITER_INSTR();
   }
   ENDOP()

   CASE(TPERS_ITER_INSTR)
   COMPLEX_JUMP(tpers_iter) {
      predicate* pred(theProgram->get_predicate(iter_predicate(pc)));
      const reg_num reg(iter_reg(pc));
      match* mobj(retrieve_match_object(state, pc, pred, TPERS_ITER_BASE));

      const return_type ret(execute_pers_iter(reg, mobj,
                                              pc + iter_inner_jump(pc), state,
                                              pred, state.sched->thread_node));

      DECIDE_NEXT_ITER_INSTR();
   }
   ENDOP()

   CASE(RLINEAR_ITER_INSTR)
   COMPLEX_JUMP(rlinear_iter) {
      predicate* pred(theProgram->get_predicate(iter_predicate(pc)));
      const reg_num reg(iter_reg(pc));
      match* mobj(retrieve_match_object(state, pc, pred, RLINEAR_ITER_BASE));

      const return_type ret(execute_rlinear_iter(
          reg, mobj, pc + iter_inner_jump(pc), state, pred, state.node));

      DECIDE_NEXT_ITER_INSTR();
   }
   ENDOP()

   CASE(OPERS_ITER_INSTR)
   COMPLEX_JUMP(opers_iter) {
      predicate* pred(theProgram->get_predicate(iter_predicate(pc)));
      const reg_num reg(iter_reg(pc));
      match* mobj(retrieve_match_object(state, pc, pred, OPERS_ITER_BASE));

      const return_type ret(execute_opers_iter(
          reg, mobj, pc, pc + iter_inner_jump(pc), state, pred, state.node));

      DECIDE_NEXT_ITER_INSTR();
   }
   ENDOP()

   CASE(OLINEAR_ITER_INSTR)
   COMPLEX_JUMP(olinear_iter) {
      predicate* pred(theProgram->get_predicate(iter_predicate(pc)));
      const reg_num reg(iter_reg(pc));
      match* mobj(retrieve_match_object(state, pc, pred, OLINEAR_ITER_BASE));

      const return_type ret(execute_olinear_iter(
          reg, mobj, pc, pc + iter_inner_jump(pc), state, pred, state.node));

      DECIDE_NEXT_ITER_INSTR();
   }
   ENDOP()

   CASE(ORLINEAR_ITER_INSTR)
   COMPLEX_JUMP(orlinear_iter) {
      predicate* pred(theProgram->get_predicate(iter_predicate(pc)));
      const reg_num reg(iter_reg(pc));
      match* mobj(retrieve_match_object(state, pc, pred, ORLINEAR_ITER_BASE));

      const return_type ret(execute_orlinear_iter(
          reg, mobj, pc, pc + iter_inner_jump(pc), state, pred, state.node));

      DECIDE_NEXT_ITER_INSTR();
   }
   ENDOP()

   CASE(REMOVE_INSTR)
   JUMP(remove, REMOVE_BASE)
   execute_remove(pc, state);
   ADVANCE()
   ENDOP()

   CASE(UPDATE_INSTR)
   JUMP(update, UPDATE_BASE)
   execute_update(pc, state);
   ADVANCE()
   ENDOP()

   CASE(ALLOC_INSTR)
   JUMP(alloc, ALLOC_BASE)
   execute_alloc(pc, state);
   ADVANCE()
   ENDOP()

   CASE(SEND_INSTR)
   JUMP(send, SEND_BASE)
   execute_send(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(THREAD_SEND_INSTR)
   JUMP(thread_send, THREAD_SEND_BASE)
   execute_thread_send(pc, state);
   ADVANCE()
   ENDOP()

   CASE(ADDLINEAR_INSTR)
   JUMP(addlinear, ADDLINEAR_BASE)
   execute_add_linear(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(ADDPERS_INSTR)
   JUMP(addpers, ADDPERS_BASE)
   execute_add_node_persistent(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(RUNACTION_INSTR)
   JUMP(runaction, RUNACTION_BASE)
   execute_run_action(pc, state);
   ADVANCE()
   ENDOP()

   CASE(ENQUEUE_LINEAR_INSTR)
   JUMP(enqueue_linear, ENQUEUE_LINEAR_BASE)
   execute_enqueue_linear(pc, state);
   ADVANCE()
   ENDOP()

   CASE(SEND_DELAY_INSTR)
   JUMP(send_delay, SEND_DELAY_BASE)
   execute_send_delay(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(NOT_INSTR)
   JUMP(not_instr, NOT_BASE)
   execute_not(pc, state);
   ADVANCE()
   ENDOP()

   CASE(TESTNIL_INSTR)
   JUMP(testnil, TESTNIL_BASE)
   execute_testnil(pc, state);
   ADVANCE()
   ENDOP()

   CASE(FLOAT_INSTR)
   JUMP(float_instr, FLOAT_BASE)
   execute_float(pc, state);
   ADVANCE()
   ENDOP()

   CASE(SELECT_INSTR)
   COMPLEX_JUMP(select)
   pc = execute_select(state.node, pc, state);
   JUMP_NEXT();
   ENDOP()

   CASE(CALL_INSTR)
   JUMP(call, CALL_BASE + call_num_args(pc) * reg_val_size)
   execute_call(pc, state);
   ADVANCE()
   ENDOP()
   CASE(CALL0_INSTR)
   JUMP(call0, CALL0_BASE)
   execute_call0(pc, state);
   ADVANCE()
   ENDOP()
   CASE(CALL1_INSTR)
   JUMP(call1, CALL1_BASE)
   execute_call1(pc, state);
   ADVANCE()
   ENDOP()
   CASE(CALL2_INSTR)
   JUMP(call2, CALL2_BASE)
   execute_call2(pc, state);
   ADVANCE()
   ENDOP()
   CASE(CALL3_INSTR)
   JUMP(call3, CALL3_BASE)
   execute_call3(pc, state);
   ADVANCE()
   ENDOP()

   CASE(RULE_INSTR)
   JUMP(rule, RULE_BASE)
   execute_rule(pc, state);
   ADVANCE()
   ENDOP()

   CASE(RULE_DONE_INSTR)
   JUMP(rule_done, RULE_DONE_BASE)
   execute_rule_done(pc, state);
   ADVANCE()
   ENDOP()

   CASE(NEW_NODE_INSTR)
   JUMP(new_node, NEW_NODE_BASE)
   execute_new_node(pc, state);
   ADVANCE()
   ENDOP()

   CASE(NEW_AXIOMS_INSTR)
   JUMP(new_axioms, new_axioms_jump(pc))
   execute_new_axioms(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(PUSH_INSTR)
   JUMP(push, PUSH_BASE)
   state.stack.push();
   ADVANCE()
   ENDOP()

   CASE(PUSHN_INSTR)
   JUMP(pushn, PUSHN_BASE)
   state.stack.push(push_n(pc));
   ADVANCE()
   ENDOP()

   CASE(POP_INSTR)
   JUMP(pop, POP_BASE)
   state.stack.pop();
   ADVANCE()
   ENDOP()

   CASE(PUSH_REGS_INSTR)
   JUMP(push_regs, PUSH_REGS_BASE)
   state.stack.push_regs(state.regs);
   ADVANCE()
   ENDOP()

   CASE(POP_REGS_INSTR)
   JUMP(pop_regs, POP_REGS_BASE)
   state.stack.pop_regs(state.regs);
   ADVANCE()
   ENDOP()

   CASE(CALLF_INSTR)
   COMPLEX_JUMP(callf) {
      const vm::callf_id id(callf_get_id(pc));
      function* fun(theProgram->get_function(id));

      pc = fun->get_bytecode();
      JUMP_NEXT();
   }
   ENDOP()

   CASE(MAKE_STRUCTR_INSTR)
   JUMP(make_structr, MAKE_STRUCTR_BASE)
   execute_make_structr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MAKE_STRUCTF_INSTR)
   JUMP(make_structf, MAKE_STRUCTF_BASE)
   execute_make_structf(pc, state);
   ADVANCE()
   ENDOP()

   CASE(STRUCT_VALRR_INSTR)
   JUMP(struct_valrr, STRUCT_VALRR_BASE)
   execute_struct_valrr(pc, state);
   ADVANCE()
   ENDOP()
   CASE(STRUCT_VALFR_INSTR)
   JUMP(struct_valfr, STRUCT_VALFR_BASE)
   execute_struct_valfr(pc, state);
   ADVANCE()
   ENDOP()
   CASE(STRUCT_VALRF_INSTR)
   JUMP(struct_valrf, STRUCT_VALRF_BASE)
   execute_struct_valrf(pc, state);
   ADVANCE()
   ENDOP()
   CASE(STRUCT_VALRFR_INSTR)
   JUMP(struct_valrfr, STRUCT_VALRFR_BASE)
   execute_struct_valrfr(pc, state);
   ADVANCE()
   ENDOP()
   CASE(STRUCT_VALFF_INSTR)
   JUMP(struct_valff, STRUCT_VALFF_BASE)
   execute_struct_valff(pc, state);
   ADVANCE()
   ENDOP()
   CASE(STRUCT_VALFFR_INSTR)
   JUMP(struct_valffr, STRUCT_VALFFR_BASE)
   execute_struct_valffr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVINTFIELD_INSTR)
   JUMP(mvintfield, MVINTFIELD_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvintfield(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVINTREG_INSTR)
   JUMP(mvintreg, MVINTREG_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvintreg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVTYPEREG_INSTR)
   JUMP(mvtypereg, MVTYPEREG_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvtypereg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVFIELDFIELD_INSTR)
   JUMP(mvfieldfield, MVFIELDFIELD_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvfieldfield(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVFIELDFIELDR_INSTR)
   JUMP(mvfieldfieldr, MVFIELDFIELD_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvfieldfieldr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVFIELDREG_INSTR)
   JUMP(mvfieldreg, MVFIELDREG_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvfieldreg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVPTRREG_INSTR)
   JUMP(mvptrreg, MVPTRREG_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvptrreg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVNILFIELD_INSTR)
   JUMP(mvnilfield, MVNILFIELD_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvnilfield(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVNILREG_INSTR)
   JUMP(mvnilreg, MVNILREG_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvnilreg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVREGFIELD_INSTR)
   JUMP(mvregfield, MVREGFIELD_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvregfield(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVREGFIELDR_INSTR)
   JUMP(mvregfieldr, MVREGFIELD_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvregfieldr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVHOSTFIELD_INSTR)
   JUMP(mvhostfield, MVHOSTFIELD_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvhostfield(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVREGCONST_INSTR)
   JUMP(mvregconst, MVREGCONST_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvregconst(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVCONSTFIELD_INSTR)
   JUMP(mvconstfield, MVCONSTFIELD_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvconstfield(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVCONSTFIELDR_INSTR)
   JUMP(mvconstfieldr, MVCONSTFIELD_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvconstfieldr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVADDRFIELD_INSTR)
   JUMP(mvaddrfield, MVADDRFIELD_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvaddrfield(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVFLOATFIELD_INSTR)
   JUMP(mvfloatfield, MVFLOATFIELD_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvfloatfield(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVFLOATREG_INSTR)
   JUMP(mvfloatreg, MVFLOATREG_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvfloatreg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVINTCONST_INSTR)
   JUMP(mvintconst, MVINTCONST_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvintconst(pc);
   ADVANCE()
   ENDOP()

   CASE(MVWORLDFIELD_INSTR)
   JUMP(mvworldfield, MVWORLDFIELD_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvworldfield(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVSTACKPCOUNTER_INSTR)
   COMPLEX_JUMP(mvstackpcounter)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   pc = execute_mvstackpcounter(pc, state);
   JUMP_NEXT();
   ENDOP()

   CASE(MVPCOUNTERSTACK_INSTR)
   JUMP(mvpcounterstack, MVPCOUNTERSTACK_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvpcounterstack(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVSTACKREG_INSTR)
   JUMP(mvstackreg, MVSTACKREG_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvstackreg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVREGSTACK_INSTR)
   JUMP(mvregstack, MVREGSTACK_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvregstack(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVADDRREG_INSTR)
   JUMP(mvaddrreg, MVADDRREG_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvaddrreg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVHOSTREG_INSTR)
   JUMP(mvhostreg, MVHOSTREG_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvhostreg(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVTHREADIDREG_INSTR)
   JUMP(mvthreadidreg, MVTHREADIDREG_BASE)
#ifdef CORE_STATISTICS
   state.state.stat_moves_executed++;
#endif
   execute_mvthreadidreg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVTHREADIDFIELD_INSTR)
   JUMP(mvthreadidfield, MVTHREADIDFIELD_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvthreadidfield(pc, state);
   ADVANCE()
   ENDOP()

   CASE(ADDRNOTEQUAL_INSTR)
   JUMP(addrnotequal, operation_size)
   execute_addrnotequal(pc, state);
   ADVANCE()
   ENDOP()

   CASE(ADDREQUAL_INSTR)
   JUMP(addrequal, operation_size)
   execute_addrequal(pc, state);
   ADVANCE()
   ENDOP()

   CASE(INTMINUS_INSTR)
   JUMP(intminus, operation_size)
   execute_intminus(pc, state);
   ADVANCE()
   ENDOP()

   CASE(INTEQUAL_INSTR)
   JUMP(intequal, operation_size)
   execute_intequal(pc, state);
   ADVANCE()
   ENDOP()

   CASE(INTNOTEQUAL_INSTR)
   JUMP(intnotequal, operation_size)
   execute_intnotequal(pc, state);
   ADVANCE()
   ENDOP()

   CASE(INTPLUS_INSTR)
   JUMP(intplus, operation_size)
   execute_intplus(pc, state);
   ADVANCE()
   ENDOP()

   CASE(INTLESSER_INSTR)
   JUMP(intlesser, operation_size)
   execute_intlesser(pc, state);
   ADVANCE()
   ENDOP()

   CASE(INTGREATEREQUAL_INSTR)
   JUMP(intgreaterequal, operation_size)
   execute_intgreaterequal(pc, state);
   ADVANCE()
   ENDOP()

   CASE(BOOLOR_INSTR)
   JUMP(boolor, operation_size)
   execute_boolor(pc, state);
   ADVANCE()
   ENDOP()

   CASE(INTLESSEREQUAL_INSTR)
   JUMP(intlesserequal, operation_size)
   execute_intlesserequal(pc, state);
   ADVANCE()
   ENDOP()

   CASE(INTGREATER_INSTR)
   JUMP(intgreater, operation_size)
   execute_intgreater(pc, state);
   ADVANCE()
   ENDOP()

   CASE(INTMUL_INSTR)
   JUMP(intmul, operation_size)
   execute_intmul(pc, state);
   ADVANCE()
   ENDOP()

   CASE(INTDIV_INSTR)
   JUMP(intdiv, operation_size)
   execute_intdiv(pc, state);
   ADVANCE()
   ENDOP()

   CASE(INTMOD_INSTR)
   JUMP(intmod, operation_size)
   execute_intmod(pc, state);
   ADVANCE()
   ENDOP()

   CASE(FLOATPLUS_INSTR)
   JUMP(floatplus, operation_size)
   execute_floatplus(pc, state);
   ADVANCE()
   ENDOP()

   CASE(FLOATMINUS_INSTR)
   JUMP(floatminus, operation_size)
   execute_floatminus(pc, state);
   ADVANCE()
   ENDOP()

   CASE(FLOATMUL_INSTR)
   JUMP(floatmul, operation_size)
   execute_floatmul(pc, state);
   ADVANCE()
   ENDOP()

   CASE(FLOATDIV_INSTR)
   JUMP(floatdiv, operation_size)
   execute_floatdiv(pc, state);
   ADVANCE()
   ENDOP()

   CASE(FLOATEQUAL_INSTR)
   JUMP(floatequal, operation_size)
   execute_floatequal(pc, state);
   ADVANCE()
   ENDOP()

   CASE(FLOATNOTEQUAL_INSTR)
   JUMP(floatnotequal, operation_size)
   execute_floatnotequal(pc, state);
   ADVANCE()
   ENDOP()

   CASE(FLOATLESSER_INSTR)
   JUMP(floatlesser, operation_size)
   execute_floatlesser(pc, state);
   ADVANCE()
   ENDOP()

   CASE(FLOATLESSEREQUAL_INSTR)
   JUMP(floatlesserequal, operation_size)
   execute_floatlesserequal(pc, state);
   ADVANCE()
   ENDOP()

   CASE(FLOATGREATER_INSTR)
   JUMP(floatgreater, operation_size)
   execute_floatgreater(pc, state);
   ADVANCE()
   ENDOP()

   CASE(FLOATGREATEREQUAL_INSTR)
   JUMP(floatgreaterequal, operation_size)
   execute_floatgreaterequal(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVREGREG_INSTR)
   JUMP(mvregreg, MVREGREG_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvregreg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(BOOLEQUAL_INSTR)
   JUMP(boolequal, operation_size)
   execute_boolequal(pc, state);
   ADVANCE()
   ENDOP()

   CASE(BOOLNOTEQUAL_INSTR)
   JUMP(boolnotequal, operation_size)
   execute_boolnotequal(pc, state);
   ADVANCE()
   ENDOP()

   CASE(HEADRR_INSTR)
   JUMP(headrr, HEADRR_BASE)
   execute_headrr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(HEADFR_INSTR)
   JUMP(headfr, HEADFR_BASE)
   execute_headfr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(HEADFF_INSTR)
   JUMP(headff, HEADFF_BASE)
   execute_headff(pc, state);
   ADVANCE()
   ENDOP()

   CASE(HEADRF_INSTR)
   JUMP(headrf, HEADRF_BASE)
   execute_headrf(pc, state);
   ADVANCE()
   ENDOP()

   CASE(HEADFFR_INSTR)
   JUMP(headffr, HEADFF_BASE)
   execute_headffr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(HEADRFR_INSTR)
   JUMP(headrfr, HEADRF_BASE)
   execute_headrfr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(TAILRR_INSTR)
   JUMP(tailrr, TAILRR_BASE)
   execute_tailrr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(TAILFR_INSTR)
   JUMP(tailfr, TAILFR_BASE)
   execute_tailfr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(TAILFF_INSTR)
   JUMP(tailff, TAILFF_BASE)
   execute_tailff(pc, state);
   ADVANCE()
   ENDOP()

   CASE(TAILRF_INSTR)
   JUMP(tailrf, TAILRF_BASE)
   execute_tailrf(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVWORLDREG_INSTR)
   JUMP(mvworldreg, MVWORLDREG_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvworldreg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVCONSTREG_INSTR)
   JUMP(mvconstreg, MVCONSTREG_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvconstreg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVINTSTACK_INSTR)
   JUMP(mvintstack, MVINTSTACK_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvintstack(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVFLOATSTACK_INSTR)
   JUMP(mvfloatstack, MVFLOATSTACK_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvfloatstack(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVARGREG_INSTR)
   JUMP(mvargreg, MVARGREG_BASE)
#ifdef CORE_STATISTICS
   state.stat.stat_moves_executed++;
#endif
   execute_mvargreg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(CONSRRR_INSTR)
   JUMP(consrrr, CONSRRR_BASE)
   execute_consrrr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(CONSRFF_INSTR)
   JUMP(consrff, CONSRFF_BASE)
   execute_consrff(pc, state);
   ADVANCE()
   ENDOP()

   CASE(CONSFRF_INSTR)
   JUMP(consfrf, CONSFRF_BASE)
   execute_consfrf(pc, state);
   ADVANCE()
   ENDOP()

   CASE(CONSFFR_INSTR)
   JUMP(consffr, CONSFFR_BASE)
   execute_consffr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(CONSRRF_INSTR)
   JUMP(consrrf, CONSRRF_BASE)
   execute_consrrf(pc, state);
   ADVANCE()
   ENDOP()

   CASE(CONSRFR_INSTR)
   JUMP(consrfr, CONSRFR_BASE)
   execute_consrfr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(CONSFRR_INSTR)
   JUMP(consfrr, CONSFRR_BASE)
   execute_consfrr(pc, state);
   ADVANCE()
   ENDOP()

   CASE(CONSFFF_INSTR)
   JUMP(consfff, CONSFFF_BASE)
   execute_consfff(pc, state);
   ADVANCE()
   ENDOP()

   CASE(CALLE_INSTR)
   JUMP(calle, CALLE_BASE * calle_num_args(pc))
   execute_calle(pc, state);
   ADVANCE()
   ENDOP()

   CASE(SET_PRIORITY_INSTR)
   JUMP(set_priority, SET_PRIORITY_BASE)
   execute_set_priority(pc, state);
   ADVANCE()
   ENDOP()

   CASE(SET_PRIORITYH_INSTR)
   JUMP(set_priorityh, SET_PRIORITYH_BASE)
   execute_set_priority_here(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(ADD_PRIORITY_INSTR)
   JUMP(add_priority, ADD_PRIORITY_BASE)
   execute_add_priority(pc, state);
   ADVANCE()
   ENDOP()

   CASE(ADD_PRIORITYH_INSTR)
   JUMP(add_priorityh, ADD_PRIORITYH_BASE)
   execute_add_priority_here(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(SET_DEFPRIOH_INSTR)
   JUMP(set_defprioh, SET_DEFPRIOH_BASE)
   execute_set_defprio_here(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(SET_DEFPRIO_INSTR)
   JUMP(set_defprio, SET_DEFPRIO_BASE)
   execute_set_defprio(pc, state);
   ADVANCE()
   ENDOP()

   CASE(SET_STATICH_INSTR)
   JUMP(set_statich, SET_STATICH_BASE)
   execute_set_static_here(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(SET_STATIC_INSTR)
   JUMP(set_static, SET_STATIC_BASE)
   execute_set_static(pc, state);
   ADVANCE()
   ENDOP()

   CASE(SET_MOVINGH_INSTR)
   JUMP(set_movingh, SET_MOVINGH_BASE)
   execute_set_moving_here(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(SET_MOVING_INSTR)
   JUMP(set_moving, SET_MOVING_BASE)
   execute_set_moving(pc, state);
   ADVANCE()
   ENDOP()

   CASE(SET_AFFINITYH_INSTR)
   JUMP(set_affinityh, SET_AFFINITYH_BASE)
   execute_set_affinity_here(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(SET_AFFINITY_INSTR)
   JUMP(set_affinity, SET_AFFINITY_BASE)
   execute_set_affinity(pc, state);
   ADVANCE()
   ENDOP()

   CASE(STOP_PROG_INSTR)
   JUMP(stop_program, STOP_PROG_BASE)
   if (scheduling_mechanism) sched::thread::stop_flag = true;
   ADVANCE()
   ENDOP()

   CASE(CPU_ID_INSTR)
   JUMP(cpu_id, CPU_ID_BASE)
   execute_cpu_id(pc, state);
   ADVANCE()
   ENDOP()

   CASE(NODE_PRIORITY_INSTR)
   JUMP(node_priority, NODE_PRIORITY_BASE)
   execute_node_priority(pc, state);
   ADVANCE()
   ENDOP()

   CASE(CPU_STATIC_INSTR)
   JUMP(cpu_static, CPU_STATIC_BASE)
   execute_cpu_static(pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVCPUSREG_INSTR)
   JUMP(mvcpusreg, MVCPUSREG_BASE)
   execute_mvcpusreg(pc, state);
   ADVANCE()
   ENDOP()

   CASE(SET_CPUH_INSTR)
   JUMP(set_cpuh, SET_CPUH_BASE)
   execute_set_cpu_here(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(MVSTACKFIELD_INSTR)
   JUMP(mvstackfield, MVSTACKFIELD_BASE)
   execute_mvstackfield(pc, state);
   ADVANCE()
   ENDOP()

   CASE(IS_STATIC_INSTR)
   JUMP(is_static, IS_STATIC_BASE)
   execute_is_static(pc, state);
   ADVANCE()
   ENDOP()

   CASE(IS_MOVING_INSTR)
   JUMP(is_moving, IS_MOVING_BASE)
   execute_is_moving(pc, state);
   ADVANCE()
   ENDOP()

   CASE(FACTS_PROVED_INSTR)
   JUMP(facts_proved, FACTS_PROVED_BASE)
   execute_facts_proved(pc, state);
   ADVANCE()
   ENDOP()

   CASE(FACTS_CONSUMED_INSTR)
   JUMP(facts_consumed, FACTS_CONSUMED_BASE)
   execute_facts_consumed(pc, state);
   ADVANCE()
   ENDOP()

   CASE(REM_PRIORITY_INSTR)
   JUMP(rem_priority, REM_PRIORITY_BASE)
   execute_rem_priority(pc, state);
   ADVANCE()
   ENDOP()

   CASE(REM_PRIORITYH_INSTR)
   JUMP(rem_priorityh, REM_PRIORITYH_BASE)
   execute_rem_priority_here(state.node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(SCHEDULE_NEXT_INSTR)
   JUMP(schedule_next, SCHEDULE_NEXT_BASE)
   execute_schedule_next(pc, state);
   ADVANCE()
   ENDOP()

   CASE(ADDTPERS_INSTR)
   JUMP(addtpers, ADDTPERS_BASE)
   execute_add_thread_persistent(state.sched->thread_node, pc, state);
   ADVANCE()
   ENDOP()

   CASE(FABS_INSTR)
   JUMP(lfabs, FABS_BASE)
   execute_fabs(pc, state);
   ADVANCE();
   ENDOP();

   CASE(REMOTE_UPDATE_INSTR)
   COMPLEX_JUMP(remote_update)
   execute_remote_update(pc, state);
   pc += REMOTE_UPDATE_BASE + remote_update_nregs(pc) * reg_val_size;
   JUMP_NEXT();
   ENDOP()

   CASE(MARK_RULE_INSTR)
   JUMP(mark_rule, MARK_RULE_BASE)
   execute_mark_rule(pc, state);
   ADVANCE();
   ENDOP();

   CASE(LITERAL_CONS_INSTR)
   COMPLEX_JUMP(literal_cons)
   execute_literal_cons(pc, state);
   pc += literal_cons_jump(pc);
   JUMP_NEXT();
   ENDOP();

   CASE(NODE_TYPE_INSTR)
   COMPLEX_JUMP(node_type)
   execute_node_type(pc, state);
   pc += NODE_TYPE_BASE + node_type_nodes(pc) * val_size;
   JUMP_NEXT();
   ENDOP();

   COMPLEX_JUMP(not_found)
#ifndef COMPUTED_GOTOS
   default:
#endif
      throw vm_exec_error("unsupported instruction");
      ENDOP()
#ifndef COMPUTED_GOTOS
}
}
#endif
}

static inline return_type do_execute(byte_code code, state& state,
                                     const reg_num reg, vm::tuple* tpl,
                                     predicate* pred) {
   assert(state.stack.empty());

   const return_type ret(execute((pcounter)code, state, reg, tpl, pred));

   assert(state.stack.empty());
   return ret;
}

execution_return execute_process(byte_code code, state& state, vm::tuple* tpl,
                                 predicate* pred) {
   state.running_rule = false;
   const return_type ret(do_execute(code, state, 0, tpl, pred));

#ifdef CORE_STATISTICS
#endif

   if (ret == RETURN_LINEAR)
      return EXECUTION_CONSUMED;
   else
      return EXECUTION_OK;
}

void execute_rule(const rule_id rule_id, state& state) {
#ifdef CORE_STATISTICS
   execution_time::scope s(state.stat.rule_times[rule_id]);
#endif
#ifdef INSTRUMENTATION
   state.instr_rules_run++;
#endif

   vm::rule* rule(theProgram->get_rule(rule_id));

   state.running_rule = true;
   do_execute(rule->get_bytecode(), state, 0, nullptr, nullptr);

#ifdef CORE_STATISTICS
   if (state.stat.stat_rules_activated == 0) state.stat.stat_rules_failed++;
#endif
}
}

#endif  // !COMPILED
