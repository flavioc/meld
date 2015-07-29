
#include <strstream>
#include "vm/predicate.hpp"
#include "vm/state.hpp"
#include "db/database.hpp"
#include "vm/priority.hpp"
#include "thread/thread.hpp"
#include "vm/instr.hpp"

static inline void execute_add_linear0(db::node *node, vm::tuple *tuple,
                                       vm::predicate *pred, vm::state &state) {
#ifdef DEBUG_SENDS
   std::cout << "\tadd linear ";
   tuple->print(std::cout, pred);
   std::cout << std::endl;
#endif

   node->add_linear_fact(tuple, pred);
   state.linear_facts_generated++;
}

static inline void execute_enqueue_linear0(vm::tuple *tuple,
                                           vm::predicate *pred,
                                           vm::state &state) {
   assert(pred->is_linear_pred());
#ifdef DEBUG_SENDS
   std::cout << "\tenqueue ";
   tuple->print(std::cout, pred);
   std::cout << std::endl;
#endif

   state.add_generated(tuple, pred);
}

static inline void execute_add_node_persistent0(db::node *node, vm::tuple *tpl,
                                                vm::predicate *pred,
                                                vm::state &state) {
#ifdef DEBUG_SENDS
   std::cout << "\tadd persistent ";
   tpl->print(std::cout, pred);
   std::cout << std::endl;
#endif

   assert(pred->is_persistent_pred() || pred->is_reused_pred());
   if (state.direction == vm::POSITIVE_DERIVATION &&
       pred->is_persistent_pred() && !pred->has_code &&
       !pred->is_aggregate_pred()) {
      if (!pred->is_compact_pred())
         node->pers_store.add_tuple(tpl, pred, state.depth);
      state.matcher->new_persistent_fact(pred->get_id());
      return;
   }
   assert(!pred->is_compact_pred());
   auto stuple(new vm::full_tuple(tpl, pred, state.direction, state.depth));
   state.add_node_persistent_fact(stuple);
}

static inline void execute_add_thread_persistent0(db::node *thread_node,
                                                  vm::tuple *tpl,
                                                  vm::predicate *pred,
                                                  vm::state &state) {
#ifdef DEBUG_SENDS
   std::cout << "\tadd thread persistent ";
   tpl->print(std::cout, pred);
   std::cout << std::endl;
#endif

   assert(pred->is_persistent_pred() || pred->is_reused_pred());
   if (state.direction == vm::POSITIVE_DERIVATION &&
       pred->is_persistent_pred() && !pred->has_code &&
       !pred->is_aggregate_pred()) {
      if (!pred->is_compact_pred())
         thread_node->pers_store.add_tuple(tpl, pred, state.depth);
      state.matcher->new_persistent_fact(pred->get_id());
      return;
   }
   auto stuple(new vm::full_tuple(tpl, pred, state.direction, state.depth));
   state.add_thread_persistent_fact(stuple);
}

static inline void execute_run_action0(vm::tuple *tpl, vm::predicate *pred,
                                       vm::state &state) {
   assert(pred->is_action_pred());
   switch (state.direction) {
      case vm::POSITIVE_DERIVATION:
         vm::All->MACHINE->run_action(state.sched, tpl, pred,
                                      &(state.node->alloc), state.gc_nodes);
         break;
      case vm::NEGATIVE_DERIVATION:
         vm::tuple::destroy(tpl, pred, &(state.node->alloc), state.gc_nodes);
         break;
   }
}

static inline void set_node_priority(vm::state &state, db::node *n,
                                     const vm::priority_t prio) {
   if (!scheduling_mechanism) return;
#ifdef COORDINATION_BUFFERING
   auto it(state.set_priorities.find(n));

   if (it == state.set_priorities.end())
      state.set_priorities[n] = prio;
   else {
      const priority_t current(it->second);
      if (higher_priority(prio, current)) it->second = prio;
   }
#else
   state.sched->set_node_priority(n, prio);
#endif
}

static inline void execute_set_priority0(vm::node_val node, vm::priority_t prio,
                                         vm::state &state) {
#ifdef USE_REAL_NODES
   db::node *n((db::node *)node);
#else
   db::node *n(vm::All->DATABASE->find_node(node));
#endif
   set_node_priority(state, n, prio);
}

static inline void execute_thread_send0(sched::thread *th, vm::tuple *tpl,
                                        vm::predicate *pred, vm::state &state) {
   if (th == state.sched) {
      if (pred->is_linear_pred())
         execute_enqueue_linear0(tpl, pred, state);
      else
         execute_add_thread_persistent0(th->thread_node, tpl, pred, state);
   } else {
      LOG_FACT_SENT();
      state.sched->new_thread_work(th, tpl, pred);
   }
}

static inline void execute_send0(db::node *from, const vm::node_val dest_val,
                                 vm::tuple *tuple, vm::predicate *pred,
                                 vm::state &state) {
   if (state.direction == vm::NEGATIVE_DERIVATION && pred->is_linear_pred() &&
       !pred->is_reused_pred()) {
      mem::node_allocator *alloc;
      if ((db::node *)dest_val == (db::node *)tuple)
         alloc = &(from->alloc);
      else
         alloc = &(((db::node *)dest_val)->alloc);
      vm::tuple::destroy(tuple, pred, alloc, state.gc_nodes);
      return;
   }

#ifdef CORE_STATISTICS
   state.stat.stat_predicate_proven[pred->get_id()]++;
#endif
   LOG_FACT_SENT();

#ifdef DEBUG_SENDS
   vm::node_val print_val(dest_val);
#ifdef USE_REAL_NODES
   print_val = ((db::node *)print_val)->get_id();
#endif
   std::cout << "\tsend ";
   tuple->print(std::cout, pred);
   std::cout << " to " << print_val << std::endl;
#endif
   if ((db::node *)tuple == (db::node *)dest_val) {
#ifdef DEBUG_SENDS
      std::cout << "\tlocal send ";
      tuple->print(std::cout, pred);
      std::cout << std::endl;
#endif
      // same node
      if (pred->is_action_pred())
         execute_run_action0(tuple, pred, state);
      else if (pred->is_persistent_pred() || pred->is_reused_pred())
         execute_add_node_persistent0(from, tuple, pred, state);
      else
         execute_enqueue_linear0(tuple, pred, state);
   } else {
      db::node *dest((db::node *)dest_val);
      if (pred->is_action_pred())
         vm::All->MACHINE->run_action(state.sched, tuple, pred,
                                      &(state.node->alloc), state.gc_nodes);
      else {
#ifdef FACT_BUFFERING
         if (pred->is_persistent_pred() || pred->is_reused_pred() ||
             state.direction != POSITIVE_DERIVATION) {
            state.sched->new_work(state.node, dest, tuple, pred,
                                  state.direction, state.depth);
         } else {
	    assert(dest);
            state.facts_to_send.add(dest, tuple, pred);
	 }
#else
         state.sched->new_work(state.node, dest, tuple, pred, state.direction,
                               state.depth);
#endif
      }
   }
}

static inline vm::tuple_field axiom_read_data(vm::pcounter &pc, vm::type *t) {
   vm::tuple_field f{0};

   switch (t->get_type()) {
      case vm::FIELD_INT:
         SET_FIELD_INT(f, vm::instr::pcounter_int(pc));
         vm::instr::pcounter_move_int(&pc);
         break;
      case vm::FIELD_FLOAT:
         SET_FIELD_FLOAT(f, vm::instr::pcounter_float(pc));
         vm::instr::pcounter_move_float(&pc);
         break;
      case vm::FIELD_NODE: {
         vm::node_val val(vm::instr::pcounter_node(pc));
         SET_FIELD_NODE(f, val);
         vm::instr::pcounter_move_node(&pc);
      } break;
      case vm::FIELD_LIST:
         if (*pc == 0) {
            pc++;
            SET_FIELD_CONS(f, runtime::cons::null_list());
         } else if (*pc == 1) {
            pc++;
            vm::list_type *lt((vm::list_type *)t);
            vm::tuple_field head(axiom_read_data(pc, lt->get_subtype()));
            vm::tuple_field tail(axiom_read_data(pc, t));
            runtime::cons *c(FIELD_CONS(tail));
            runtime::cons *nc(
                runtime::cons::create(c, head, lt->get_subtype()));
            SET_FIELD_CONS(f, nc);
         } else {
            assert(false);
         }
         break;
      case FIELD_STRUCT: {
         struct_type *st((struct_type *)t);
         runtime::struct1 *s(runtime::struct1::create(st));
         for (size_t i(0); i < st->get_size(); ++i) {
            tuple_field data(axiom_read_data(pc, st->get_type(i)));
            s->set_data(i, data, st);
         }
         SET_FIELD_STRUCT(f, s);
      } break;
      default:
         assert(false);
         abort();
   }

   return f;
}

static inline void tuple_set_field(vm::tuple *tpl, vm::type *t,
                                   const field_num i, const tuple_field field) {
   switch (t->get_type()) {
      case FIELD_LIST:
         tpl->set_cons(i, FIELD_CONS(field));
         break;
      case FIELD_STRUCT:
         tpl->set_struct(i, FIELD_STRUCT(field));
         break;
      case FIELD_NODE:
         tpl->set_node(i, FIELD_NODE(field));
         break;
      case FIELD_INT:
      case FIELD_FLOAT:
         tpl->set_field(i, field);
         break;
      case FIELD_STRING:
         tpl->set_string(i, FIELD_STRING(field));
         break;
      default:
         abort();
         break;
   }
}

static inline void add_new_axioms(state &state, db::node *node, pcounter pc,
                                  const pcounter end) {
   while (pc < end) {
      // read axioms until the end!
      const vm::uint_val num(vm::instr::pcounter_int(pc));
      vm::instr::pcounter_move_int(&pc);
      predicate_id pid(vm::instr::predicate_get(pc, 0));
      predicate *pred(theProgram->get_predicate(pid));
      vm::instr::pcounter_move_byte(&pc);
      if (pred->is_persistent_pred() && pred->is_compact_pred()) {
         db::array *s(node->pers_store.get_array(pred));
         s->init(num, pred, &(node->alloc));
         for (size_t j(0); j < num; ++j) {
            tuple *tpl(s->add_next(pred));
            for (size_t i(0), num_fields(pred->num_fields()); i != num_fields;
                 ++i) {
               tuple_field field(axiom_read_data(pc, pred->get_field_type(i)));
               tuple_set_field(tpl, pred->get_field_type(i), i, field);
            }
         }
         state.matcher->new_persistent_fact(pred->get_id());
      } else {
         for (size_t j(0); j < num; ++j) {
            tuple *tpl(vm::tuple::create(pred, &(node->alloc)));
            for (size_t i(0), num_fields(pred->num_fields()); i != num_fields;
                 ++i) {
               tuple_field field(axiom_read_data(pc, pred->get_field_type(i)));
               tuple_set_field(tpl, pred->get_field_type(i), i, field);
            }

            if (pred->is_action_pred())
               execute_run_action0(tpl, pred, state);
            else if (pred->is_reused_pred() || pred->is_persistent_pred()) {
               if (pred->is_persistent_pred() && !pred->has_code &&
                   !pred->is_aggregate_pred()) {
                  node->pers_store.add_tuple(tpl, pred, state.depth);
                  state.matcher->new_persistent_fact(pred->get_id());
               } else
                  execute_add_node_persistent0(node, tpl, pred, state);
            } else
               execute_add_linear0(node, tpl, pred, state);
         }
      }
   }
}

#ifdef COMPILED
INCBIN_EXTERN(Axioms);

static inline void read_axioms(state &state, db::node *node,
                               const size_t table) {
   uint32_t *tbl = (uint32_t *)(((utils::byte *)gAxiomsData) + table);

   tbl += node->get_id() * 2;
   const uint32_t start = *tbl;
   const uint32_t len = *(tbl + 1);

   if (start == 0 || len == 0) return;

   utils::byte *data = (utils::byte *)gAxiomsData + start;

   add_new_axioms(state, node, data, data + len);
}

static inline int_val read_node_type(db::node *node, const uint32_t offset, const size_t num_nodes)
{
   if(node->get_id() >= num_nodes)
      return 0;

   unsigned char *table((pcounter)(((utils::byte *)gAxiomsData) + offset));

   return (int_val)table[node->get_id()];
}

static inline void do_fix_nodes(db::node *node, const size_t table) {
   uint32_t *tbl = (uint32_t *)(((utils::byte *)gAxiomsData) + table);

   tbl += node->get_id() * 2;
   const uint32_t start = *tbl;
   const uint32_t len = *(tbl + 1);

   if (start == 0 || len == 0) return;

   utils::byte *data = ((utils::byte *)gAxiomsData);
   for (size_t i(start); i < start + len; i += sizeof(vm::code_offset_t)) {
      vm::code_offset_t off(*(vm::code_offset_t *)(data + i));
      vm::node_val *p((vm::node_val *)(data + off));
      vm::instr::pcounter_set_node((utils::byte *)p, (node_val)node);
   }
}

static inline vm::tuple_field instantiate_data(const uint32_t offset, vm::type *typ) {
   pcounter p((pcounter)(((utils::byte *)gAxiomsData) + offset));
   return axiom_read_data(p, typ);
}

std::unique_ptr<std::istrstream> compiled_database_stream() {
   return std::unique_ptr<std::istrstream>(
       new std::istrstream((const char *)gAxiomsData, gAxiomsSize));
}

#endif
