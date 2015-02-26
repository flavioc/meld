
static inline void
execute_add_linear0(db::node *node, tuple *tuple, predicate *pred, state& state)
{
#ifdef DEBUG_SENDS
   std::cout << "\tadd linear ";
   tuple->print(std::cout, pred);
   std::cout << std::endl;
#endif

   node->add_linear_fact(tuple, pred);
   state.linear_facts_generated++;
}

static inline void
execute_enqueue_linear0(db::node *node, vm::tuple *tuple, predicate *pred, state& state)
{
   assert(pred->is_linear_pred());
#ifdef DEBUG_SENDS
   std::cout << "\tenqueue ";
   tuple->print(std::cout, pred);
   std::cout << std::endl;
#endif

   node->store.add_generated(tuple, pred);
   state.generated_facts = true;
   state.linear_facts_generated++;
}

static inline void
execute_add_persistent0(db::node *node, vm::tuple *tpl, predicate *pred, state& state)
{
#ifdef DEBUG_SENDS
   std::cout << "\tadd persistent ";
   tpl->print(std::cout, pred);
   std::cout << std::endl;
#endif

   assert(pred->is_persistent_pred() || pred->is_reused_pred());
   full_tuple *stuple(new full_tuple(tpl, pred, state.direction, state.depth));
   node->store.persistent_tuples.push_back(stuple);
   state.persistent_facts_generated++;
}

static inline void
execute_run_action0(vm::tuple *tpl, predicate *pred, state& state)
{
   assert(pred->is_action_pred());
   switch(state.direction) {
      case POSITIVE_DERIVATION:
         All->MACHINE->run_action(state.sched, tpl, pred, state.gc_nodes);
         break;
      case NEGATIVE_DERIVATION:
         vm::tuple::destroy(tpl, pred, state.gc_nodes);
         break;
   }
}

static inline void
execute_set_priority0(vm::node_val node, vm::priority_t prio, vm::state& state)
{
#ifdef USE_REAL_NODES
   db::node *n((db::node*)node);
#else
   db::node *n(All->DATABASE->find_node(node));
#endif

#ifdef COORDINATION_BUFFERING
   auto it(state.set_priorities.find(n));

   if(it == state.set_priorities.end())
      state.set_priorities[n] = prio;
   else {
      const priority_t current(it->second);
      if(higher_priority(prio, current))
         it->second = prio;
   }
#else
   state.sched->set_node_priority(n, prio);
#endif
}

static inline void
execute_send0(db::node *node, const vm::node_val dest_val, vm::tuple *tuple, vm::predicate *pred, vm::state& state)
{
   if(state.direction == NEGATIVE_DERIVATION && pred->is_linear_pred() && !pred->is_reused_pred()) {
      vm::tuple::destroy(tuple, pred, state.gc_nodes);
      return;
   }

#ifdef CORE_STATISTICS
   state.stat.stat_predicate_proven[pred->get_id()]++;
#endif
#ifdef FACT_STATISTICS
   state.facts_sent++;
#endif

#ifdef DEBUG_SENDS
   vm::node_val print_val(dest_val);
#ifdef USE_REAL_NODES
   print_val = ((db::node*)print_val)->get_id();
#endif
   std::cout << "\tsend ";
   tuple->print(std::cout, pred);
   std::cout << " to " << print_val << std::endl;
#endif
#ifdef USE_REAL_NODES
   if(node == (db::node*)dest_val)
#else
   if(state.node->get_id() == dest_val)
#endif
   {
#ifdef DEBUG_SENDS
      std::cout << "\tlocal send ";
      tuple->print(std::cout, pred);
      std::cout << std::endl;
#endif

      // same node
      if(pred->is_action_pred())
         execute_run_action0(tuple, pred, state);
      else if(pred->is_persistent_pred() || pred->is_reused_pred())
         execute_add_persistent0(node, tuple, pred, state);
      else
         execute_enqueue_linear0(node, tuple, pred, state);
   } else {
#ifdef USE_REAL_NODES
      db::node *node((db::node*)dest_val);
#else
      db::node *node(All->DATABASE->find_node(dest_val));
#endif
      if(pred->is_action_pred())
         All->MACHINE->run_action(state.sched, tuple, pred, state.gc_nodes);
      else {
#ifdef FACT_BUFFERING
         auto it(state.facts_to_send.find(node));
         tuple_array *arr;
         if(it == state.facts_to_send.end()) {
            state.facts_to_send.insert(make_pair(node, tuple_array()));
            it = state.facts_to_send.find(node);
         }
         arr = &(it->second);
         full_tuple info(tuple, pred, state.direction, state.depth);
         arr->push_back(info);
#else
         state.sched->new_work(state.node, node, tuple, pred, state.direction, state.depth);
#endif
      }
   }
}

static inline tuple_field
axiom_read_data(pcounter& pc, type *t)
{
   tuple_field f;

   switch(t->get_type()) {
      case FIELD_INT:
         SET_FIELD_INT(f, vm::instr::pcounter_int(pc));
         vm::instr::pcounter_move_int(&pc);
         break;
      case FIELD_FLOAT:
         SET_FIELD_FLOAT(f, vm::instr::pcounter_float(pc));
         vm::instr::pcounter_move_float(&pc);
         break;
      case FIELD_NODE: {
         vm::node_val val(vm::instr::pcounter_node(pc));
         SET_FIELD_NODE(f, val);
         vm::instr::pcounter_move_node(&pc);
      }
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
            runtime::cons *nc(runtime::cons::create(c, head, lt));
            SET_FIELD_CONS(f, nc);
         } else {
            assert(false);
         }
         break;
      case FIELD_STRUCT: {
         struct_type *st((struct_type*)t);
         runtime::struct1 *s(runtime::struct1::create(st));
         for(size_t i(0); i < st->get_size(); ++i) {
            tuple_field data(axiom_read_data(pc, st->get_type(i)));
            s->set_data(i, data);
         }
         SET_FIELD_STRUCT(f, s);
      }
      break;
      default: assert(false);
   }

   return f;
}

static inline void
tuple_set_field(vm::tuple *tpl, vm::type *t, const field_num i, const tuple_field field)
{
   switch(t->get_type()) {
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

static inline void
add_new_axioms(state& state, db::node *node, pcounter pc, const pcounter end)
{
   while(pc < end) {
      // read axions until the end!
      predicate_id pid(vm::instr::predicate_get(pc, 0));
      predicate *pred(theProgram->get_predicate(pid));
      tuple *tpl(vm::tuple::create(pred));
#ifdef FACT_STATISTICS
      state.facts_derived++;
#endif

      pc++;

      for(size_t i(0), num_fields(pred->num_fields());
            i != num_fields;
            ++i)
      {
         tuple_field field(axiom_read_data(pc, pred->get_field_type(i)));

         tuple_set_field(tpl, pred->get_field_type(i), i, field);
      }

      if(pred->is_action_pred())
         execute_run_action0(tpl, pred, state);
      else if(pred->is_reused_pred() || pred->is_persistent_pred())
         execute_add_persistent0(node, tpl, pred, state);
      else
         execute_add_linear0(node, tpl, pred, state);
   }
}

#ifdef COMPILED
INCBIN_EXTERN(Axioms);

static inline void
read_new_axioms(state& state, db::node *node, const size_t start, const size_t len)
{
   utils::byte *data = ((utils::byte*)gAxiomsData) + start;
   add_new_axioms(state, node, data, data + len);
}

static inline void
do_fix_nodes(db::node *n, const size_t start, const size_t len)
{
   utils::byte *data = ((utils::byte*)gAxiomsData);
   for(size_t i(start); i < start + len; i += sizeof(vm::code_offset_t)) {
      vm::code_offset_t off(*(vm::code_offset_t*)(data + i));
      vm::node_val *p((vm::node_val*)(data + off));
      vm::instr::pcounter_set_node((utils::byte*)p, (node_val)n);
   }
}
#endif
