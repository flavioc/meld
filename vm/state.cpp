
#include <queue>

#include "vm/state.hpp"
#include "machine.hpp"
#include "vm/exec.hpp"

using namespace vm;
using namespace db;
using namespace process;
using namespace std;
using namespace runtime;
using namespace utils;

//#define DEBUG_DB
//#define DEBUG_INDEXING

extern void run_rule(state *s, db::node *n, db::node *t, size_t);
extern void run_predicate(state *s, vm::tuple *, db::node *n, db::node *t,
                          const vm::predicate_id);

namespace vm {

#ifdef DYNAMIC_INDEXING
static volatile deterministic_timestamp indexing_epoch(0);
static size_t run_node_calls(0);

static enum {
   INDEXING_COUNTS,
   GUESSING_FIELDS,
   ADD_INDEXES,
   DONE_INDEXING
} indexing_phase = INDEXING_COUNTS;
#endif

void state::purge_runtime_objects(void) {
   using runtime::array;
#define PURGE_OBJ_SIMPLE(TYPE)    \
   for (TYPE * x : free_##TYPE) { \
      assert(x != nullptr);       \
      x->dec_refs();              \
   }                              \
   free_##TYPE.clear()

   PURGE_OBJ_SIMPLE(rstring);
   for (auto p : free_cons) p.first->dec_refs(p.second, gc_nodes);
   free_cons.clear();
   for (auto p : free_struct1) p.first->dec_refs(p.second, gc_nodes);
   free_struct1.clear();
   for (auto p : free_array) p.first->dec_refs(p.second, gc_nodes);
   free_array.clear();
   for (auto p : free_set) p.first->dec_refs(p.second, gc_nodes);
   free_set.clear();
}

void state::cleanup(void)
{
   purge_runtime_objects();
#ifndef COMPILED
   updated_map.clear();
   tuple_regs.clear();
#endif
}

#ifndef COMPILED
void state::copy_reg2const(const reg_num &reg_from, const const_id &cid) {
   All->set_const(cid, regs[reg_from]);
   switch (theProgram->get_const_type(cid)->get_type()) {
      case FIELD_LIST:
         runtime::cons::inc_refs(All->get_const_cons(cid));
         break;
      case FIELD_STRING:
         All->get_const_string(cid)->inc_refs();
         break;
      default:
         break;
   }
}
#endif

void state::setup(vm::predicate *pred, const derivation_direction dir,
                  const depth_t dpt) {
   direction = dir;
   if (pred != nullptr) {
      if (pred->is_cycle_pred())
         depth = dpt + 1;
      else
         depth = 0;
   } else
      depth = 0;
#ifndef COMPILED
   if (pred != nullptr)
      is_linear = pred->is_linear_pred();
   else
      is_linear = false;
#endif
#ifdef CORE_STATISTICS
   stat.start_matching();
#endif
}

static inline bool tuple_for_assertion(full_tuple *stpl) {
   return (stpl->get_predicate()->is_aggregate_pred() &&
           stpl->is_aggregate()) ||
          !stpl->get_predicate()->is_aggregate_pred();
}

full_tuple *state::search_for_negative_tuple(vm::full_tuple_list *ls,
                                             full_tuple *stpl) {
   vm::tuple *tpl(stpl->get_tuple());
   vm::predicate *pred(stpl->get_predicate());

   for (auto it(ls->begin()), end(ls->end()); it != end; ++it) {
      full_tuple *stpl2(*it);
      vm::tuple *tpl2(stpl2->get_tuple());
      vm::predicate *pred2(stpl2->get_predicate());

      if (pred == pred2 && stpl2->is_aggregate() == stpl->is_aggregate() &&
          stpl2->get_dir() == NEGATIVE_DERIVATION && tpl2->equal(*tpl, pred)) {
         ls->erase(it);
         return stpl2;
      }
   }

   return nullptr;
}

inline void
state::do_persistent_tuples(db::node *node, vm::full_tuple_list *ls) {
#if !defined(COMPILED) || defined(COMPILED_DERIVES_PERSISTENT)
   while (!ls->empty()) {
      full_tuple *stpl(ls->pop_front());
      vm::predicate *pred(stpl->get_predicate());
      vm::tuple *tpl(stpl->get_tuple());

      if (pred->is_persistent_pred() &&
          stpl->get_dir() == POSITIVE_DERIVATION) {
         full_tuple *stpl2(search_for_negative_tuple(ls, stpl));
         if (stpl2) {
            assert(stpl != stpl2);
            assert(stpl2->get_tuple() != stpl->get_tuple());
            assert(stpl2->get_predicate() == stpl->get_predicate());
            full_tuple::wipeout(stpl, &(node->alloc), gc_nodes);
            full_tuple::wipeout(stpl2, &(node->alloc), gc_nodes);
            continue;
         }
      }

      if (tuple_for_assertion(stpl)) {
#ifdef DEBUG_RULES
         cout << ">>>>>>>>>>>>> Running process for " << node->get_id() << " "
              << *stpl << " (" << stpl->get_depth() << ")" << endl;
#endif
         process_persistent_tuple(node, stpl, tpl);
      } else {
// aggregate
#ifdef DEBUG_RULES
         cout << ">>>>>>>>>>>>> Adding aggregate " << node->get_id() << " "
              << *stpl << " (" << stpl->get_depth() << ")" << endl;
#endif
         add_to_aggregate(node, ls, stpl);
      }
   }
   assert(ls->empty());
#else
   (void)ls;
   (void)node;
#endif
}

inline void
state::process_action_tuples(db::node *node) {
   for (full_tuple *stpl : node->store.incoming_action_tuples) {
      vm::tuple *tpl(stpl->get_tuple());
      vm::predicate *pred(stpl->get_predicate());
      All->MACHINE->run_action(sched, tpl, pred, &(node->alloc), gc_nodes);
      delete stpl;
   }
   node->store.incoming_action_tuples.clear();
}

inline void
state::process_incoming_tuples(db::node *node) {
   for (size_t i(0); i < theProgram->num_linear_predicates(); ++i) {
      utils::intrusive_list<vm::tuple> *ls(node->store.get_incoming(i));
      if (!ls->empty()) {
         vm::predicate *pred(theProgram->get_linear_predicate(i));
         matcher->new_linear_fact(pred->get_id());
         node->linear.increment_database(pred, ls, &(node->alloc));
      }
   }
}

void state::add_to_aggregate(db::node *node, vm::full_tuple_list *ls,
                             full_tuple *stpl) {
#ifndef COMPILED_NO_AGGREGATES
   vm::tuple *tpl(stpl->get_tuple());
   predicate *pred(stpl->get_predicate());
   agg_configuration *agg(nullptr);

   switch (stpl->get_dir()) {
      case vm::NEGATIVE_DERIVATION:
         agg = node->pers_store.remove_agg_tuple(tpl, stpl->get_predicate(),
                                                 stpl->get_depth(),
                                                 &(node->alloc), gc_nodes);
         break;
      case vm::POSITIVE_DERIVATION:
         agg = node->pers_store.add_agg_tuple(
             tpl, stpl->get_predicate(), stpl->get_depth(),
             vm::POSITIVE_DERIVATION, &(node->alloc), gc_nodes);
         break;
   }

   full_tuple_list list;

   agg->generate(pred, pred->get_aggregate_type(), pred->get_aggregate_field(),
                 list, &(node->alloc));

   for (full_tuple_list::iterator it(list.begin()); it != list.end(); ++it) {
      full_tuple *stpl(*it);
      stpl->set_as_aggregate();
   }
   ls->splice_back(list);
#else
   (void)node;
   (void)stpl;
   (void)ls;
#endif
}

void state::process_persistent_tuple(db::node *target_node, full_tuple *stpl,
                                     vm::tuple *tpl) {
   predicate *pred(stpl->get_predicate());

   // persistent tuples are marked inside this loop
   switch (stpl->get_dir()) {
      case POSITIVE_DERIVATION: {
         bool is_new;

         if (pred->is_reused_pred())
            is_new = true;
         else {
#ifdef CORE_STATISTICS
            execution_time::scope s(
                stat.db_insertion_time_predicate[pred->get_id()]);
#endif
            is_new = target_node->pers_store.add_tuple(tpl, pred, depth);
         }

         if (is_new) {
            setup(pred, POSITIVE_DERIVATION, stpl->get_depth());
            if (pred->has_code)
#ifdef COMPILED
               run_predicate(this, tpl, node, sched->thread_node,
                             pred->get_id());
#else
               execute_process(
                   theProgram->get_predicate_bytecode(pred->get_id()), *this,
                   tpl, pred);
#endif
         }

         if (pred->is_reused_pred())
            target_node->add_linear_fact(stpl->get_tuple(), pred);
         else {
            matcher->new_persistent_fact(pred->get_id());

            if (!is_new)
               vm::tuple::destroy(tpl, pred, &(target_node->alloc), gc_nodes);
         }

         delete stpl;
      } break;
      case NEGATIVE_DERIVATION:
         if (pred->is_reused_pred()) {
            setup(pred, NEGATIVE_DERIVATION, stpl->get_depth());
            if (pred->has_code)
#ifdef COMPILED
               run_predicate(this, tpl, node, sched->thread_node,
                             pred->get_id());
#else
               execute_process(
                   theProgram->get_predicate_bytecode(pred->get_id()), *this,
                   tpl, pred);
#endif
         } else {
            auto deleter(target_node->pers_store.delete_tuple(
                tpl, pred, stpl->get_depth()));

            if (!deleter.is_valid()) {
               // do nothing... it does not exist
            } else if (deleter.to_delete()) {  // to be removed
               setup(pred, NEGATIVE_DERIVATION, stpl->get_depth());
               if (pred->has_code)
#ifdef COMPILED
                  run_predicate(this, tpl, node, sched->thread_node,
                                pred->get_id());
#else
                  execute_process(
                      theProgram->get_predicate_bytecode(pred->get_id()), *this,
                      tpl, pred);
#endif
               deleter.perform_delete(pred, &(target_node->alloc), gc_nodes);
            } else if (pred->is_cycle_pred()) {
               depth_counter *dc(deleter.get_depth_counter());
               assert(dc != nullptr);

               if (dc->get_count(stpl->get_depth()) == 0) {
                  deleter.delete_depths_above(stpl->get_depth());
                  if (deleter.to_delete()) {
                     setup(pred, stpl->get_dir(), stpl->get_depth());
                     if (pred->has_code)
#ifdef COMPILED
                        run_predicate(this, tpl, node, sched->thread_node,
                                      pred->get_id());
#else
                        execute_process(
                            theProgram->get_predicate_bytecode(pred->get_id()),
                            *this, tpl, pred);
#endif
                     deleter.perform_delete(pred, &(target_node->alloc),
                                            gc_nodes);
                  }
               }
            }
            matcher->new_persistent_count(pred->get_id(), deleter.trie_size());
         }
         vm::full_tuple::wipeout(stpl, &(target_node->alloc), gc_nodes);
         break;
   }
}

#ifdef DYNAMIC_INDEXING
static vector<pair<predicate *, size_t>> one_indexing_fields;
static vector<pair<predicate *, size_t>> two_indexing_fields;
static vector<size_t> indexing_scores;

static inline void find_fields_to_improve_index(vm::counter *match_counter) {
   for (size_t i(0); i < theProgram->num_predicates(); ++i) {
      predicate *pred(theProgram->get_predicate(i));
      if (pred->is_persistent_pred()) continue;
      const int start(pred->get_argument_position());
      const int end(start + pred->num_fields() - 1);
      priority_queue<pair<size_t, size_t>,
                     std::vector<pair<size_t, size_t>,
                                 mem::allocator<pair<size_t, size_t>>>> queue;
      for (int j(start); j <= end; ++j) {
         const size_t count(match_counter->get_count((size_t)j));
         if (count > 0) queue.push(make_pair(count, j));
      }
      if (!queue.empty()) {
         if (queue.size() == 1) {
            // select one
            pair<size_t, size_t> fst(queue.top());
            one_indexing_fields.push_back(make_pair(pred, fst.second - start));
         } else {
            // select best two
            pair<size_t, size_t> fst(queue.top());
            queue.pop();
            pair<size_t, size_t> snd(queue.top());
            two_indexing_fields.push_back(make_pair(pred, fst.second - start));
            two_indexing_fields.push_back(make_pair(pred, snd.second - start));
            indexing_scores.push_back(0);
            indexing_scores.push_back(0);
         }
      }
   }
#ifdef DEBUG_INDEXING
   for (size_t i(0); i < two_indexing_fields.size(); ++i) {
      pair<predicate *, size_t> p(two_indexing_fields[i]);
      cout << p.first->get_name() << " " << p.second << endl;
   }
#endif
}

static unordered_map<vm::int_val, size_t> count_ints;
static unordered_map<vm::float_val, size_t> count_floats;
static unordered_map<vm::node_val, size_t> count_nodes;

static inline double compute_entropy(db::node *node, const predicate *pred,
                                     const size_t arg) {
   double ret = 0.0;
   size_t total(0);

   assert(count_ints.empty());
   assert(count_floats.empty());
   assert(count_nodes.empty());

   if (node->linear.stored_as_hash_table(pred)) {
      // XXX TODO
   } else {
      utils::intrusive_list<tuple> *ls(
          node->linear.get_linked_list(pred->get_linear_id()));

      for (vm::tuple *tpl : *ls) {
         total++;
         switch (pred->get_field_type(arg)->get_type()) {
            case FIELD_INT: {
               const int_val val(tpl->get_int(arg));
               unordered_map<int_val, size_t>::iterator it(
                   count_ints.find(val));
               if (it == count_ints.end())
                  count_ints[val] = 1;
               else
                  it->second++;
            } break;
            case FIELD_FLOAT: {
               const float_val val(tpl->get_float(arg));
               unordered_map<float_val, size_t>::iterator it(
                   count_floats.find(val));
               if (it == count_floats.end())
                  count_floats[val] = 1;
               else
                  it->second++;
            } break;
            case FIELD_NODE: {
               node_val val(tpl->get_node(arg));
#ifdef USE_REAL_NODES
               val = ((db::node *)val)->get_id();
#endif
               unordered_map<node_val, size_t>::iterator it(
                   count_nodes.find(val));
               if (it == count_nodes.end())
                  count_nodes[val] = 1;
               else
                  it->second++;
            } break;
            default:
               throw vm_exec_error("type not implemented");
               assert(false);
               break;
         }
      }
   }

   double all((double)total);
   if (all <= 0.0) return ret;

   switch (pred->get_field_type(arg)->get_type()) {
      case FIELD_INT: {
         for (unordered_map<int_val, size_t>::iterator it(count_ints.begin()),
              end(count_ints.end());
              it != end; ++it) {
            double items((double)it->second);
            ret += items / all * log2(items / all);
         }
         count_ints.clear();
      } break;
      case FIELD_FLOAT: {
         for (unordered_map<float_val, size_t>::iterator it(
                  count_floats.begin()),
              end(count_floats.end());
              it != end; ++it) {
            double items((double)it->second);
            ret += items / all * log2(items / all);
         }
         count_floats.clear();
      } break;
      case FIELD_NODE: {
         for (unordered_map<node_val, size_t>::iterator it(count_nodes.begin()),
              end(count_nodes.end());
              it != end; ++it) {
            double items((double)it->second);
            ret += items / all * log2(items / all);
         }
         count_nodes.clear();
      } break;
      default:
         assert(false);
         break;
   }

   return -ret;
}

static inline void gather_indexing_stats_about_node(db::node *node,
                                                    vm::counter *counter) {
   for (size_t i(0); i < two_indexing_fields.size(); i += 2) {
      const pair<predicate *, size_t> p1(two_indexing_fields[i]);
      const predicate *pred(p1.first);
      const size_t start(pred->get_argument_position());
      const size_t arg1(p1.second);
      const pair<predicate *, size_t> p2(two_indexing_fields[i + 1]);
      const size_t arg2(p2.second);
      const double entropy1(compute_entropy(node, pred, arg1));
      const double entropy2(compute_entropy(node, pred, arg2));
      const double count1((double)counter->get_count(start + arg1));
      const double count2((double)counter->get_count(start + arg2));
      assert(count1 > 0.0);
      assert(count2 > 0.0);
      const double res1(entropy1 * -log2(1.0 / count1));
      const double res2(entropy2 * -log2(1.0 / count2));

#ifdef DEBUG_INDEXING
      cout << pred->get_name() << " " << arg1 << " -> " << entropy1 << "  VS "
           << arg2 << " -> " << entropy2 << endl;
      cout << "count " << arg1 << " -> " << count1 << "  VS  " << arg2 << " -> "
           << count2 << endl;
      cout << "res " << arg1 << " -> " << res1 << "  VS  " << arg2 << " -> "
           << res2 << endl;
#endif

      if (res1 > res2)
         indexing_scores[i]++;
      else if (res2 > res1)
         indexing_scores[i + 1]++;
   }
}

void state::indexing_state_machine(db::node *no) {
#ifdef DYNAMIC_INDEXING
   switch (indexing_phase) {
      case INDEXING_COUNTS: {
         const size_t target(
             max(max((size_t)100, All->DATABASE->num_nodes() / 100),
                 All->DATABASE->num_nodes() / (25 * All->NUM_THREADS)));
         run_node_calls++;
         if (run_node_calls >= target) {
#ifdef DEBUG_INDEXING
            cout << "Computing fields in " << run_node_calls << " calls\n";
#endif
            run_node_calls = 0;
            find_fields_to_improve_index(match_counter);
            if (!two_indexing_fields.empty())
               indexing_phase = GUESSING_FIELDS;
            else {
               if (one_indexing_fields.empty())
                  indexing_phase = DONE_INDEXING;
               else
                  indexing_phase = ADD_INDEXES;
            }
         }
      } break;
      case GUESSING_FIELDS: {
         // perform specific work on nodes
         const size_t target(max(All->DATABASE->num_nodes() / 100,
                                 max((size_t)50, All->DATABASE->num_nodes() /
                                                     (25 * All->NUM_THREADS))));
         run_node_calls++;
         gather_indexing_stats_about_node(no, match_counter);
         if (run_node_calls >= target) {
#ifdef DEBUG_INDEXING
            cout << "Indexing phase " << run_node_calls << endl;
#endif
            run_node_calls = 0;
            indexing_phase = ADD_INDEXES;
            // fall through
         } else
            break;
      }
      case ADD_INDEXES: {
         bool different(false);
         for (size_t i(0); i < one_indexing_fields.size(); ++i) {
            pair<predicate *, size_t> p(one_indexing_fields[i]);
            predicate *pred(p.first);
            const size_t arg(p.second);
#ifdef DEBUG_INDEXING
            cout << "Hash in " << pred->get_name() << " " << arg << endl;
#endif
            if (pred->is_hash_table()) {
               const field_num old(pred->get_hashed_field());
               if (old != arg) {
                  different = true;
                  pred->store_as_hash_table(arg);
               }
            } else {
               different = true;
               pred->store_as_hash_table(arg);
            }
         }
         for (size_t i(0); i < indexing_scores.size(); i += 2) {
            size_t score1(indexing_scores[i]);
            size_t score2(indexing_scores[i + 1]);
            size_t arg1(two_indexing_fields[i].second);
            size_t arg2(two_indexing_fields[i + 1].second);
            predicate *pred(two_indexing_fields[i].first);
            const size_t start(pred->get_argument_position());
            if (score1 < score2) {
               swap(arg1, arg2);
               swap(score1, score2);
            } else if (score1 == score2) {
// pick the one with the most counts
#ifdef DEBUG_INDEX
               cout << "Same score" << endl;
#endif
               if (match_counter->get_count(start + arg2) >
                   match_counter->get_count(start + arg1)) {
                  swap(arg1, arg2);
                  swap(score1, score2);
               }
            }
#ifdef DEBUG_INDEXING
            cout << "For predicate " << pred->get_name() << " pick " << arg1
                 << endl;
            cout << arg1 << " " << match_counter->get_count(start + arg1) << " "
                 << arg2 << " " << match_counter->get_count(start + arg2)
                 << endl;
#endif
            // arg1 is the best
            if (pred->is_hash_table()) {
               const field_num old(pred->get_hashed_field());
               if (old != arg1) {
                  different = true;
                  pred->store_as_hash_table(arg1);
               }
            } else {
               different = true;
               pred->store_as_hash_table(arg1);
            }
// TODO: remove indexes?
#ifdef DEBUG_INDEXING
            cout << *pred << " " << arg1 << " " << score1 << " vs " << arg2
                 << " " << score2 << endl;
#endif
         }
         if (different) indexing_epoch++;
         indexing_phase = DONE_INDEXING;
      } break;
      case DONE_INDEXING:
         break;
   }
#else
   (void)no;
#endif
}
#endif

void state::run_node(db::node *node) {
   this->node = node;
   this->matcher = &(node->matcher);

   reset_counters();
   assert(node_persistent_tuples.empty());
   assert(thread_persistent_tuples.empty());

#ifdef DYNAMIC_INDEXING
   if (sched && sched->get_id() == 0) indexing_state_machine(no);
#endif

#ifdef DEBUG_RULES
   cout << "===============================================================\n";
   cout << "===                                                         ===\n";
   cout << "===                  NODE " << node->get_id()
        << "                                ===\n";
   cout << "===                                                         ===\n";
#ifdef DEBUG_DB
   node->print(cout);
#endif
   cout << "===============================================================\n";
#endif

   LOCK_STACK(node_lock);
   MUTEX_LOCK(node->main_lock, node_lock, node_lock);
   LOCK_STACK(internal_lock_data);
   MUTEX_LOCK(node->database_lock, internal_lock_data, database_lock);

   {
      process_action_tuples(node);
      process_incoming_tuples(node);
#if !defined(COMPILED) || defined(COMPILED_DERIVES_PERSISTENT)
      if(!node->store.incoming_persistent_tuples.empty())
         node_persistent_tuples.splice_back(
             node->store.incoming_persistent_tuples);
#endif

#ifdef DYNAMIC_INDEXING
      if (node->indexing_epoch != indexing_epoch) {
         node->linear.rebuild_index();
         node->indexing_epoch = indexing_epoch;
      }
#endif

      node->unprocessed_facts = false;
      MUTEX_UNLOCK(node->main_lock, node_lock);
      // incoming facts have been processed, we release the main lock
      // but the database locks remains locked.
      do_persistent_tuples(node, &node_persistent_tuples);
   }

//#if !defined(COMPILED) || defined(COMPILED_THREAD_FACTS)
   // add linear facts to the node matcher
   if (theProgram->has_thread_predicates() && sched->thread_node != node) {
      LOCK_STACK(thread_node_lock);
      MUTEX_LOCK(sched->thread_node->main_lock, thread_node_lock, node_lock);
      process_action_tuples(sched->thread_node);
      process_incoming_tuples(sched->thread_node);
      thread_persistent_tuples.splice_back(
          sched->thread_node->store.incoming_persistent_tuples);
      sched->thread_node->unprocessed_facts = false;
      MUTEX_UNLOCK(sched->thread_node->main_lock, thread_node_lock);
      do_persistent_tuples(sched->thread_node, &thread_persistent_tuples);
      matcher->add_thread(sched->thread_node->matcher);
   }
//#endif

   while (!matcher->rule_queue.empty(theProgram->num_rules_next_uint())) {
      rule_id rule(
          matcher->rule_queue.remove_front(theProgram->num_rules_next_uint()));

#ifdef DEBUG_RULES
      cout << "Run rule " << theProgram->get_rule(rule)->get_string() << endl;
#endif

      setup(nullptr, POSITIVE_DERIVATION, 0);
      generated_facts = false;
#ifdef COMPILED
      run_rule(this, node, sched->thread_node, rule);
#else
      execute_rule(rule, *this);
#endif

      // move from generated tuples to linear store
      if (generated_facts) {
         for (size_t i(0); i < theProgram->num_linear_predicates(); ++i) {
            utils::intrusive_list<vm::tuple> *gen(get_generated(i));
            if (!gen->empty()) {
               vm::predicate *pred(theProgram->get_linear_predicate(i));
               if (pred->is_thread_pred()) {
                  matcher->new_linear_fact(pred->get_id());
                  sched->thread_node->linear.increment_database(pred, gen, &(node->alloc));
               } else {
                  matcher->new_linear_fact(pred->get_id());
                  node->linear.increment_database(pred, gen, &(node->alloc));
               }
            }
         }
      }
      do_persistent_tuples(node, &node_persistent_tuples);
      assert(node_persistent_tuples.empty());
//#if !defined(COMPILED) || defined(COMPILED_THREAD_FACTS)
      if (theProgram->has_thread_predicates() && node != sched->thread_node)
         do_persistent_tuples(sched->thread_node, &thread_persistent_tuples);
//#endif

#if defined(COMPILED_CHANGES_OWNER) or !defined(COMPILED)
      if (node->has_new_owner()) {
         node->unprocessed_facts = true;
         break;
      }
#endif

#ifdef DEBUG_DB
      node->print(cout);
#endif
   }

//#if !defined(COMPILED) || defined(COMPILED_THREAD_FACTS)
   // unmark all thread facts and save state in 'thread_node'.
   if (theProgram->has_thread_predicates() && sched->thread_node != node)
      matcher->remove_thread(sched->thread_node->matcher);
//#endif

   assert(node_persistent_tuples.empty());
   assert(thread_persistent_tuples.empty());
   node->manage_index();
   MUTEX_UNLOCK(node->database_lock, internal_lock_data);
//#if !defined(COMPILED) || defined(COMPILED_THREAD_FACTS)
   if (theProgram->has_thread_predicates() && sched->thread_node != node)
      sched->thread_node->manage_index();
//#endif

   sync(node);

#ifdef GC_NODES
   for (auto x : gc_nodes) {
      db::node *n((db::node *)x);
      LOCK_STACK(nodelock);
      MUTEX_LOCK(n->main_lock, nodelock, node_lock);
      // need to lock node since it may have pending facts
      if (n->garbage_collect()) {
         sched->delete_node(n);
         // node cannot be unlocked since it is gone.
      } else {
         MUTEX_UNLOCK(n->main_lock, nodelock);
      }
   }
   gc_nodes.clear();
#endif
   cleanup();
}

inline bool
state::sync(db::node *node) {
   bool ret(false);
#ifdef FACT_BUFFERING
   // send all facts to nodes.
   for (size_t i(0); i < facts_to_send.size_initial; ++i) {
      buffer_node &ls(facts_to_send.initial[i].b);
      db::node *target(facts_to_send.initial[i].node);
      assert(target);
      sched->new_work_list(node, target, ls);
      ret = true;
   }
   if (facts_to_send.lots_of_nodes()) {
      for (auto p : facts_to_send.facts) {
         buffer_node &ls(p.second);
         db::node *target((db::node *)p.first);
         sched->new_work_list(node, target, ls);
         ret = true;
      }
   }
   facts_to_send.clear();
#endif
#ifdef COORDINATION_BUFFERING
   for (auto p : set_priorities) {
      db::node *target((db::node *)p.first);
      sched->set_node_priority(target, p.second);
   }
   set_priorities.clear();
#endif
   (void)node;
   return ret;
}

void state::reset_counters() {
   linear_facts_generated = 0;
   persistent_facts_generated = 0;
   linear_facts_consumed = 0;
}

state::state(sched::thread *_sched)
    : sched(_sched)
#ifdef CORE_STATISTICS
      ,
      stat()
#endif
{
#ifndef COMPILED
   if (sched) {
      generated = mem::allocator<tuple_list>().allocate(
          vm::theProgram->num_linear_predicates());
      for (size_t i(0); i < vm::theProgram->num_linear_predicates(); ++i)
         mem::allocator<tuple_list>().construct(generated + i);
   }
#endif
#ifdef DYNAMIC_INDEXING
   if (sched) {
      match_counter = create_counter(theProgram->get_total_arguments());
      if (sched->get_id() == 0) {
         indexing_epoch = 0;
         run_node_calls = 0;
      }
   }
#endif
   cleanup();
}

state::state() : state(nullptr) {}

state::~state(void) {
#ifdef CORE_STATISTICS
   if (sched != nullptr) stat.print(cout);
#endif
   if (match_counter) {
      // cout << "==================================\n";
      // match_counter->print(theProgram->get_total_arguments());
      delete_counter(match_counter, theProgram->get_total_arguments());
   }
#ifndef COMPILED
   for (utils::byte *obj : allocated_match_objects)
      mem::allocator<utils::byte>().deallocate(
          obj, MATCH_OBJECT_SIZE * All->NUM_THREADS);
   if (generated) {
      for (size_t i(0); i < vm::theProgram->num_linear_predicates(); ++i)
         mem::allocator<tuple_list>().destroy(generated + i);
      mem::allocator<tuple_list>().deallocate(
          generated, vm::theProgram->num_linear_predicates());
   }
#endif
}
}
