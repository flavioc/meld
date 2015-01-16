
static inline void
execute_enqueue_linear0(db::node *node, vm::tuple *tuple, predicate *pred, state& state)
{
   assert(pred->is_linear_pred());
#ifdef DEBUG_SENDS
   cout << "\tenqueue ";
   tuple->print(cout, pred);
   cout << endl;
#endif

   node->store.add_generated(tuple, pred);
   state.generated_facts = true;
   state.linear_facts_generated++;
}

static inline void
execute_add_persistent0(db::node *node, vm::tuple *tpl, predicate *pred, state& state)
{
#ifdef DEBUG_SENDS
   cout << "\tadd persistent ";
   tpl->print(cout, pred);
   cout << endl;
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
   {
      MUTEX_LOCK_GUARD(print_mtx, print_lock);
      ostringstream ss;
      node_val print_val(dest_val);
#ifdef USE_REAL_NODES
      print_val = ((db::node*)print_val)->get_id();
#endif
      ss << "\t";
      tuple->print(ss, pred);
      ss << " " << (state.direction == POSITIVE_DERIVATION ? "+" : "-") << " -> " << print_val << " (" << state.depth << ")" << endl;
      cout << ss.str();
   }
#endif
#ifdef USE_REAL_NODES
   if(node == (db::node*)dest_val)
#else
   if(state.node->get_id() == dest_val)
#endif
   {
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

