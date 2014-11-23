
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

//#define DEBUG_RULES
//#define DEBUG_INDEXING

namespace vm
{

#ifdef USE_UI
bool state::UI = false;
#endif

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

void
state::purge_runtime_objects(void)
{
#define PURGE_OBJ(TYPE)                                                                      \
   for(list<TYPE*>::iterator it(free_ ## TYPE.begin()); it != free_ ## TYPE .end(); ++it) {  \
      TYPE *x(*it);                                                                          \
      assert(x != NULL);                                                                     \
      x->dec_refs(gc_nodes);                                                                 \
   }                                                                                         \
   free_ ## TYPE .clear()
#define PURGE_OBJ_SIMPLE(TYPE)                                                               \
   for(list<TYPE*>::iterator it(free_ ## TYPE.begin()); it != free_ ## TYPE .end(); ++it) {  \
      TYPE *x(*it);                                                                          \
      assert(x != NULL);                                                                     \
      x->dec_refs();                                                                         \
   }                                                                                         \
   free_ ## TYPE .clear()

   PURGE_OBJ(cons);
	PURGE_OBJ_SIMPLE(rstring);
   PURGE_OBJ(struct1);
	
#undef PURGE_OBJ
}

void
state::cleanup(void)
{
   purge_runtime_objects();
   removed.clear();
}

void
state::copy_reg2const(const reg_num& reg_from, const const_id& cid)
{
   All->set_const(cid, regs[reg_from]);
	switch(theProgram->get_const_type(cid)->get_type()) {
		case FIELD_LIST:
         runtime::cons::inc_refs(All->get_const_cons(cid)); break;
		case FIELD_STRING:
			All->get_const_string(cid)->inc_refs(); break;
		default: break;
	}
}

void
state::setup(vm::predicate *pred, db::node *n, const derivation_count count, const depth_t depth)
{
   this->node = n;
   this->count = count;
   if(pred != NULL) {
      if(pred->is_cycle_pred())
         this->depth = depth + 1;
      else
         this->depth = 0;
   } else {
      this->depth = 0;
   }
	if(pred != NULL)
   	this->is_linear = pred->is_linear_pred();
	else
		this->is_linear = false;
   for(size_t i(0); i < NUM_REGS; ++i) {
      this->saved_leaves[i] = NULL;
      this->is_leaf[i] = false;
   }
#ifdef CORE_STATISTICS
   this->stat.start_matching();
#endif
}

void
state::mark_active_rules(void)
{
   for(bitmap::iterator it(store->matcher.predicates.begin(theProgram->num_predicates()));
         !it.end(); it++)
   {
      const predicate_id p(*it);
      predicate *pred(theProgram->get_predicate(p));
      rule_queue.set_bits_of_and_result(pred->get_rules_map(), store->matcher.active_bitmap,
            theProgram->num_rules_next_uint());
   }
	
   rule_queue.unset_bits(store->matcher.dropped_bitmap, theProgram->num_rules_next_uint());
   store->matcher.clear_dropped_rules();
}

bool
state::add_fact_to_node(vm::tuple *tpl, vm::predicate *pred, const vm::derivation_count count, const vm::depth_t depth)
{
#ifdef CORE_STATISTICS
   execution_time::scope s(stat.db_insertion_time_predicate[tpl->get_predicate_id()]);
#endif

	return node->add_tuple(tpl, pred, count, depth);
}

static inline bool
tuple_for_assertion(full_tuple *stpl)
{
   return (stpl->get_predicate()->is_aggregate_pred() && stpl->is_aggregate())
      || !stpl->get_predicate()->is_aggregate_pred();
}

full_tuple*
state::search_for_negative_tuple_partial_agg(full_tuple *stpl)
{
   vm::tuple *tpl(stpl->get_tuple());
   vm::predicate *pred(stpl->get_predicate());

   assert(!stpl->is_aggregate());

   for(auto it(store->persistent_tuples.begin()),
         end(store->persistent_tuples.end());
         it != end; ++it)
   {
      full_tuple *stpl2(*it);
      vm::tuple *tpl2(stpl2->get_tuple());
      vm::predicate *pred2(stpl2->get_predicate());

      if(pred == pred2 && !stpl2->is_aggregate() &&
            stpl2->get_count() == -1 && tpl2->equal(*tpl, pred))
      {
         store->persistent_tuples.erase(it);
         return stpl2;
      }
   }

   return NULL;
}

full_tuple*
state::search_for_negative_tuple_normal(full_tuple *stpl)
{
   vm::tuple *tpl(stpl->get_tuple());
   vm::predicate *pred1(stpl->get_predicate());

   assert(!pred1->is_aggregate_pred());
   assert(!stpl->is_aggregate());

   for(auto it(store->persistent_tuples.begin()),
         end(store->persistent_tuples.end());
         it != end; ++it)
   {
      full_tuple *stpl2(*it);
      vm::tuple *tpl2(stpl2->get_tuple());
      vm::predicate* pred2(stpl2->get_predicate());

      if(pred1 == pred2 && stpl2->get_count() == -1 && tpl2->equal(*tpl, pred1))
      {
         store->persistent_tuples.erase(it);
         return stpl2;
      }
   }

   return NULL;
}

full_tuple*
state::search_for_negative_tuple_full_agg(full_tuple *stpl)
{
   vm::tuple *tpl(stpl->get_tuple());
   vm::predicate *pred1(stpl->get_predicate());

   for(auto it(store->persistent_tuples.begin()), end(store->persistent_tuples.end());
         it != end; ++it)
   {
      full_tuple *stpl2(*it);
      vm::tuple *tpl2(stpl2->get_tuple());
      vm::predicate *pred2(stpl2->get_predicate());

      if(pred1 == pred2 && stpl2->is_aggregate() && stpl2->get_count() == -1 && tpl2->equal(*tpl, pred1))
      {
         store->persistent_tuples.erase(it);
         return stpl2;
      }
   }

   return NULL;
}

void
state::do_persistent_tuples(void)
{
   while(!store->persistent_tuples.empty()) {
      full_tuple *stpl(store->persistent_tuples.pop_front());
      vm::predicate *pred(stpl->get_predicate());
      vm::tuple *tpl(stpl->get_tuple());

      if(pred->is_persistent_pred()) {
         if(stpl->get_count() == 1 && (pred->is_aggregate_pred() && !stpl->is_aggregate())) {
            full_tuple *stpl2(search_for_negative_tuple_partial_agg(stpl));
            if(stpl2) {
               assert(stpl != stpl2);
               assert(stpl2->get_tuple() != stpl->get_tuple());
               assert(stpl2->get_predicate() == stpl->get_predicate());
               full_tuple::wipeout(stpl
#ifdef GC_NODES
                     , gc_nodes
#endif
                     );
               full_tuple::wipeout(stpl2
#ifdef GC_NODES
                     , gc_nodes
#endif
                     );
               continue;
            }
         }

         if(stpl->get_count() == 1 && (pred->is_aggregate_pred() && stpl->is_aggregate())) {
            full_tuple *stpl2(search_for_negative_tuple_full_agg(stpl));
            if(stpl2) {
               assert(stpl != stpl2);
               assert(stpl2->get_tuple() != stpl->get_tuple());
               assert(stpl2->get_predicate() == stpl->get_predicate());
               full_tuple::wipeout(stpl
#ifdef GC_NODES
                     , gc_nodes
#endif
                     );
               full_tuple::wipeout(stpl2
#ifdef GC_NODES
                     , gc_nodes
#endif
                     );
               continue;
            }
         }

         if(stpl->get_count() == 1 && !pred->is_aggregate_pred()) {
            full_tuple *stpl2(search_for_negative_tuple_normal(stpl));
            if(stpl2) {
               full_tuple::wipeout(stpl
#ifdef GC_NODES
                     , gc_nodes
#endif
                     );
               full_tuple::wipeout(stpl2
#ifdef GC_NODES
                     , gc_nodes
#endif
                     );
               continue;
            }
         }
      }

      if(tuple_for_assertion(stpl)) {
#ifdef DEBUG_RULES
      cout << ">>>>>>>>>>>>> Running process for " << node->get_id() << " " << *stpl << " (" << stpl->get_depth() << ")" << endl;
#endif
         process_persistent_tuple(stpl, tpl);
      } else {
         // aggregate
#ifdef DEBUG_RULES
      cout << ">>>>>>>>>>>>> Adding aggregate " << node->get_id() << " " << *stpl << " (" << stpl->get_depth() << ")" << endl;
#endif
         add_to_aggregate(stpl);
      }
   }
}

void
state::process_action_tuples(void)
{
   store->action_tuples.splice_back(store->incoming_action_tuples);
   for(auto it(store->action_tuples.begin()), end(store->action_tuples.end());
         it != end; ++it)
   {
      full_tuple *stpl(*it);
      vm::tuple *tpl(stpl->get_tuple());
      vm::predicate *pred(stpl->get_predicate());
      All->MACHINE->run_action(sched, node, tpl, pred
#ifdef GC_NODES
            , gc_nodes
#endif
            );
      delete stpl;
   }
   store->action_tuples.clear();
}

void
state::process_incoming_tuples(void)
{
#ifdef UNIQUE_INCOMING_LIST
   while(!store->incoming.empty()) {
      vm::tuple *tpl(store->incoming.pop_front());
      vm::predicate *pred(tpl->get_predicate());
      store->register_tuple_fact(pred, 1);
      lstore->add_fact(tpl, pred, store->matcher);
   }
#else
   for(size_t i(0); i < theProgram->num_predicates(); ++i) {
      utils::intrusive_list<vm::tuple> *ls(store->incoming + i);
      if(!ls->empty()) {
         vm::predicate *pred(theProgram->get_predicate(i));
         store->register_tuple_fact(pred, ls->get_size());
         lstore->increment_database(pred, ls, store->matcher);
      }
   }
#endif
   if(!store->incoming_persistent_tuples.empty())
      store->persistent_tuples.splice_back(store->incoming_persistent_tuples);
}

void
state::add_to_aggregate(full_tuple *stpl)
{
   vm::tuple *tpl(stpl->get_tuple());
   predicate *pred(stpl->get_predicate());
   vm::derivation_count count(stpl->get_count());
   agg_configuration *agg(NULL);

   if(count < 0) {
      agg = node->remove_agg_tuple(tpl, stpl->get_predicate(), -count, stpl->get_depth()
#ifdef GC_NODES
            , gc_nodes
#endif
            );
   } else {
      agg = node->add_agg_tuple(tpl, stpl->get_predicate(), count, stpl->get_depth()
#ifdef GC_NODES
            , gc_nodes
#endif
            );
   }

   full_tuple_list list;

   agg->generate(pred, pred->get_aggregate_type(), pred->get_aggregate_field(), list
#ifdef GC_NODES
         , gc_nodes
#endif
         );

   for(full_tuple_list::iterator it(list.begin()); it != list.end();) {
      full_tuple *stpl(*it);
      it++;

      stpl->set_as_aggregate();
      store->persistent_tuples.push_back(stpl);
   }
}

void
state::process_persistent_tuple(full_tuple *stpl, vm::tuple *tpl)
{
   predicate *pred(stpl->get_predicate());

   // persistent tuples are marked inside this loop
   if(stpl->get_count() > 0) {
      bool is_new;

      if(pred->is_reused_pred()) {
         is_new = true;
      } else {
         is_new = add_fact_to_node(tpl, pred, stpl->get_count(), stpl->get_depth());
      }

      if(is_new) {
         setup(pred, node, stpl->get_count(), stpl->get_depth());
         execute_process(theProgram->get_predicate_bytecode(pred->get_id()), *this, tpl, pred);
      }

      if(pred->is_reused_pred()) {
         node->add_linear_fact(stpl->get_tuple(), pred);
      } else {
         store->matcher.register_tuple(pred, stpl->get_count(), is_new);

         if(!is_new) {
            vm::tuple::destroy(tpl, pred
#ifdef GC_NODES
                  , gc_nodes
#endif
                  );
         }
      }

      delete stpl;
   } else {
		if(pred->is_reused_pred()) {
			setup(pred, node, stpl->get_count(), stpl->get_depth());
			execute_process(theProgram->get_predicate_bytecode(pred->get_id()), *this, tpl, pred);
		} else {
      	node::delete_info deleter(node->delete_tuple(tpl, pred, -stpl->get_count(), stpl->get_depth()));

         if(!deleter.is_valid()) {
            // do nothing... it does not exist
         } else if(deleter.to_delete()) { // to be removed
         	setup(pred, node, stpl->get_count(), stpl->get_depth());
         	execute_process(theProgram->get_predicate_bytecode(pred->get_id()), *this, tpl, pred);
#ifdef GC_NODES
            deleter.perform_delete(pred, gc_nodes);
#else
            deleter.perform_delete(pred);
#endif
      	} else if(pred->is_cycle_pred()) {
            depth_counter *dc(deleter.get_depth_counter());
            assert(dc != NULL);

            if(dc->get_count(stpl->get_depth()) == 0) {
               vm::derivation_count deleted(deleter.delete_depths_above(stpl->get_depth()));
               (void)deleted;
               if(deleter.to_delete()) {
                  setup(pred, node, stpl->get_count(), stpl->get_depth());
                  execute_process(theProgram->get_predicate_bytecode(pred->get_id()), *this, tpl, pred);
#ifdef GC_NODES
                  deleter.perform_delete(pred, gc_nodes);
#else
                  deleter.perform_delete(pred);
#endif
               }
            }
         }
         store->matcher.set_count(pred, deleter.trie_size());
      }
      vm::full_tuple::wipeout(stpl
#ifdef GC_NODES
            , gc_nodes
#endif
         );
   }
}

#ifdef DYNAMIC_INDEXING
static vector< pair<predicate*, size_t> > one_indexing_fields;
static vector< pair<predicate*, size_t> > two_indexing_fields;
static vector<size_t> indexing_scores;

static inline void
find_fields_to_improve_index(vm::counter *match_counter)
{
   for(size_t i(0); i < theProgram->num_predicates(); ++i) {
      predicate *pred(theProgram->get_predicate(i));
      if(pred->is_persistent_pred())
         continue;
      const int start(pred->get_argument_position());
      const int end(start + pred->num_fields() - 1);
      priority_queue< pair<size_t, size_t>, std::vector< pair<size_t, size_t>, mem::allocator< pair<size_t, size_t> > > > queue;
      for(int j(start); j <= end; ++j) {
         const size_t count(match_counter->get_count((size_t)j));
         if(count > 0)
            queue.push(make_pair(count, j));
      }
      if(!queue.empty()) {
         if(queue.size() == 1) {
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
   for(size_t i(0); i < two_indexing_fields.size(); ++i) {
      pair<predicate*, size_t> p(two_indexing_fields[i]);
      cout << p.first->get_name() << " " << p.second << endl;
   }
#endif
}

static unordered_map<vm::int_val, size_t> count_ints;
static unordered_map<vm::float_val, size_t> count_floats;
static unordered_map<vm::node_val, size_t> count_nodes;

static inline double
compute_entropy(db::node *node, const predicate *pred, const size_t arg)
{
   double ret = 0.0;
   size_t total(0);

   assert(count_ints.empty());
   assert(count_floats.empty());
   assert(count_nodes.empty());

   if(node->linear.stored_as_hash_table(pred)) {
      // XXX TODO
   } else {
      utils::intrusive_list<tuple> *ls(node->linear.get_linked_list(pred->get_id()));

      for(utils::intrusive_list<tuple>::iterator it(ls->begin()), end(ls->end()); it != end; ++it) {
         vm::tuple *tpl(*it);
         total++;
         switch(pred->get_field_type(arg)->get_type()) {
            case FIELD_INT: {
               const int_val val(tpl->get_int(arg));
               unordered_map<int_val, size_t>::iterator it(count_ints.find(val));
               if(it == count_ints.end())
                  count_ints[val] = 1;
               else
                  it->second++;
            }
            break;
            case FIELD_FLOAT: {
               const float_val val(tpl->get_float(arg));
               unordered_map<float_val, size_t>::iterator it(count_floats.find(val));
               if(it == count_floats.end())
                  count_floats[val] = 1;
               else
                  it->second++;
            }
            break;
            case FIELD_NODE: {
               node_val val(tpl->get_node(arg));
#ifdef USE_REAL_NODES
               val = ((db::node*)val)->get_id();
#endif
               unordered_map<node_val, size_t>::iterator it(count_nodes.find(val));
               if(it == count_nodes.end())
                  count_nodes[val] = 1;
               else
                  it->second++;
            }
            break;
            default: throw vm_exec_error("type not implemented"); assert(false); break;
         }
      }
   }

   double all((double)total);
   if(all <= 0.0)
      return ret;

   switch(pred->get_field_type(arg)->get_type()) {
      case FIELD_INT: {
         for(unordered_map<int_val, size_t>::iterator it(count_ints.begin()), end(count_ints.end()); it != end; ++it) {
            double items((double)it->second);
            ret += items/all * log2(items/all);
         }
         count_ints.clear();
      }
      break;
      case FIELD_FLOAT: {
         for(unordered_map<float_val, size_t>::iterator it(count_floats.begin()), end(count_floats.end()); it != end; ++it) {
            double items((double)it->second);
            ret += items/all * log2(items/all);
         }
         count_floats.clear();
      }
      break;
      case FIELD_NODE: {
         for(unordered_map<node_val, size_t>::iterator it(count_nodes.begin()), end(count_nodes.end()); it != end; ++it) {
            double items((double)it->second);
            ret += items/all * log2(items/all);
         }
         count_nodes.clear();
      }
      break;
      default: assert(false); break;
   }

   return -ret;
}

static inline void
gather_indexing_stats_about_node(db::node *node, vm::counter *counter)
{
   for(size_t i(0); i < two_indexing_fields.size(); i += 2) {
      const pair<predicate*, size_t> p1(two_indexing_fields[i]);
      const predicate *pred(p1.first);
      const size_t start(pred->get_argument_position());
      const size_t arg1(p1.second);
      const pair<predicate*, size_t> p2(two_indexing_fields[i+1]);
      const size_t arg2(p2.second);
      const double entropy1(compute_entropy(node, pred, arg1));
      const double entropy2(compute_entropy(node, pred, arg2));
      const double count1((double)counter->get_count(start + arg1));
      const double count2((double)counter->get_count(start + arg2));
      assert(count1 > 0.0);
      assert(count2 > 0.0);
      const double res1(entropy1 * -log2(1.0/count1));
      const double res2(entropy2 * -log2(1.0/count2));

#ifdef DEBUG_INDEXING
      cout << pred->get_name() << " " << arg1 << " -> " << entropy1 << "  VS " << arg2 << " -> " << entropy2 << endl;
      cout << "count " << arg1 << " -> " << count1 << "  VS  " << arg2 << " -> " << count2 << endl;
      cout << "res " << arg1 << " -> " << res1 << "  VS  " << arg2 << " -> " << res2 << endl;
#endif
         
      if(res1 > res2)
         indexing_scores[i]++;
      else if(res2 > res1)
         indexing_scores[i + 1]++;
   }
}

void
state::indexing_state_machine(db::node *no)
{
#ifdef DYNAMIC_INDEXING
   switch(indexing_phase) {
      case INDEXING_COUNTS: {
         const size_t target(max(max((size_t)100, All->DATABASE->num_nodes() / 100), All->DATABASE->num_nodes() / (25 * All->NUM_THREADS)));
         run_node_calls++;
         if(run_node_calls >= target) {
#ifdef DEBUG_INDEXING
            cout << "Computing fields in " << run_node_calls << " calls\n";
#endif
            run_node_calls = 0;
            find_fields_to_improve_index(match_counter);
            if(!two_indexing_fields.empty())
               indexing_phase = GUESSING_FIELDS;
            else {
               if(one_indexing_fields.empty())
                  indexing_phase = DONE_INDEXING;
               else
                  indexing_phase = ADD_INDEXES;
            }
         }
       }
      break;
      case GUESSING_FIELDS: {
         // perform specific work on nodes
         const size_t target(max(All->DATABASE->num_nodes() / 100, max((size_t)50, All->DATABASE->num_nodes() / (25 * All->NUM_THREADS))));
         run_node_calls++;
         gather_indexing_stats_about_node(no, match_counter);
         if(run_node_calls >= target) {
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
         for(size_t i(0); i < one_indexing_fields.size(); ++i) {
            pair<predicate*, size_t> p(one_indexing_fields[i]);
            predicate *pred(p.first);
            const size_t arg(p.second);
#ifdef DEBUG_INDEXING
            cout << "Hash in " << pred->get_name() << " " << arg << endl;
#endif
            if(pred->is_hash_table()) {
               const field_num old(pred->get_hashed_field());
               if(old != arg) {
                  different = true;
                  pred->store_as_hash_table(arg);
               }
            } else {
               different = true;
               pred->store_as_hash_table(arg);
            }
         }
         for(size_t i(0); i < indexing_scores.size(); i += 2) {
            size_t score1(indexing_scores[i]);
            size_t score2(indexing_scores[i + 1]);
            size_t arg1(two_indexing_fields[i].second);
            size_t arg2(two_indexing_fields[i + 1].second);
            predicate *pred(two_indexing_fields[i].first);
            const size_t start(pred->get_argument_position());
            if(score1 < score2) {
               swap(arg1, arg2);
               swap(score1, score2);
            } else if(score1 == score2) {
               // pick the one with the most counts
#ifdef DEBUG_INDEX               
               cout << "Same score" << endl;
#endif
               if(match_counter->get_count(start + arg2) > match_counter->get_count(start + arg1)) {
                  swap(arg1, arg2);
                  swap(score1, score2);
               }
            }
#ifdef DEBUG_INDEXING
            cout << "For predicate " << pred->get_name() << " pick " << arg1 << endl;
            cout << arg1 << " " << match_counter->get_count(start + arg1) << " " << arg2 << " " << match_counter->get_count(start + arg2) << endl;
#endif
            // arg1 is the best
            if(pred->is_hash_table()) {
               const field_num old(pred->get_hashed_field());
               if(old != arg1) {
                  different = true;
                  pred->store_as_hash_table(arg1);
               }
            } else {
               different = true;
               pred->store_as_hash_table(arg1);
            }
            // TODO: remove indexes?
#ifdef DEBUG_INDEXING
            cout << *pred << " " << arg1 << " " << score1 << " vs " << arg2 << " " << score2 << endl;
#endif
         }
         if(different)
            indexing_epoch++;
         indexing_phase = DONE_INDEXING;
      }
      break;
      case DONE_INDEXING: break;
   }
#else
   (void)no;
#endif
}
#endif

void
state::run_node(db::node *no)
{
#ifdef DYNAMIC_INDEXING
   if(sched && sched->get_id() == 0)
      indexing_state_machine(no);
#endif

	node = no;
#ifdef DEBUG_RULES
   cout << "================> NODE " << node->get_id() << " ===============\n";
#endif

   store = &(node->store);
   lstore = &(node->linear);

	{
#ifdef CORE_STATISTICS
		execution_time::scope s(stat.core_engine_time);
#endif
      LOCK_STACK(node_lock);
      MUTEX_LOCK(node->main_lock, node_lock, node_lock);
      process_action_tuples();
		process_incoming_tuples();
#ifdef DYNAMIC_INDEXING
      if(node->indexing_epoch != indexing_epoch) {
         lstore->rebuild_index();
         node->indexing_epoch = indexing_epoch;
      }
#endif
      node->unprocessed_facts = false;
      MUTEX_UNLOCK(node->main_lock, node_lock);
	}

   LOCK_STACK(internal_lock_data);
   MUTEX_LOCK(node->database_lock, internal_lock_data, database_lock);

#ifdef DYNAMIC_INDEXING
   node->rounds++;
#endif
	
   do_persistent_tuples();
   {
#ifdef CORE_STATISTICS
		execution_time::scope s(stat.core_engine_time);
#endif
      mark_active_rules();
   }

	while(!rule_queue.empty(theProgram->num_rules_next_uint())) {
		rule_id rule(rule_queue.remove_front(theProgram->num_rules_next_uint()));
		
#ifdef DEBUG_RULES
      cout << "Run rule " << theProgram->get_rule(rule)->get_string() << endl;
#endif

		// delete rule and every check
		{
#ifdef CORE_STATISTICS
			execution_time::scope s(stat.core_engine_time);
#endif
         store->matcher.clear_predicates();
		}
		
		setup(NULL, node, 1, 0);
		execute_rule(rule, *this);

      // move from generated tuples to linear store
      if(generated_facts) {
         for(size_t i(0); i < theProgram->num_predicates(); ++i) {
            utils::intrusive_list<vm::tuple> *gen(store->generated + i);
            if(!gen->empty()) {
               vm::predicate *pred(theProgram->get_predicate(i));
               lstore->increment_database(pred, gen, store->matcher);
            }
         }
      }
      do_persistent_tuples();

		{
#ifdef CORE_STATISTICS
			execution_time::scope s(stat.core_engine_time);
#endif
			mark_active_rules();
		}

      if(node->has_new_owner())
         break;
	}

   assert(store->persistent_tuples.empty());
#ifdef DYNAMIC_INDEXING
   lstore->improve_index();
   if(node->rounds > 0 && node->rounds % 5 == 0)
      lstore->cleanup_index();
#endif
   MUTEX_UNLOCK(node->database_lock, internal_lock_data);
   sync();
#ifdef GC_NODES
   for(auto it(gc_nodes.begin()); it != gc_nodes.end(); ++it) {
      db::node *n((db::node*)*it);
      MUTEX_LOCK_GUARD(n->main_lock, node_lock);
      // need to lock node since it may have pending facts
      if(n->garbage_collect())
         sched->delete_node(n);
   }
   gc_nodes.clear();
#endif
}

bool
state::sync(void)
{
   bool ret(false);
#ifdef FACT_BUFFERING
   // send all facts to nodes.
   for(auto it(facts_to_send.begin()), end(facts_to_send.end()); it != end; ++it) {
      tuple_array& ls(it->second);
      db::node *target((db::node*)it->first);
      sched->new_work_list(node, target, ls);
      ret = true;
   }
   facts_to_send.clear();
#endif
#ifdef COORDINATION_BUFFERING
   for(auto it(set_priorities.begin()), end(set_priorities.end()); it != end; ++it) {
      db::node *target((db::node*)it->first);
      sched->set_node_priority(target, it->second);
   }
   set_priorities.clear();
#endif
   return ret;
}

state::state(sched::threads_sched *_sched):
   sched(_sched)
#ifdef DEBUG_MODE
   , print_instrs(false)
#endif
#ifdef CORE_STATISTICS
   , stat()
#endif
#ifdef FACT_STATISTICS
   , facts_derived(0)
   , facts_consumed(0)
   , facts_sent(0)
#endif
{
   bitmap::create(rule_queue, theProgram->num_rules_next_uint());
   rule_queue.clear(theProgram->num_rules_next_uint());
#ifdef INSTRUMENTATION
   instr_facts_consumed = 0;
   instr_facts_derived = 0;
   instr_rules_run = 0;
#endif
   match_counter = create_counter(theProgram->get_total_arguments());
#ifdef DYNAMIC_INDEXING
   if(sched->get_id() == 0) {
      indexing_epoch = 0;
      run_node_calls = 0;
   }
#endif
}

state::state(void):
   sched(NULL)
#ifdef DEBUG_MODE
   , print_instrs(false)
#endif
   , match_counter(NULL)
#ifdef CORE_STATISTICS
   , stat()
#endif
#ifdef FACT_STATISTICS
   , facts_derived(0)
   , facts_consumed(0)
   , facts_sent(0)
#endif
{
}

state::~state(void)
{
#ifdef CORE_STATISTICS
	if(sched != NULL)
      stat.print(cout);
#endif
   if(sched != NULL) {
      bitmap::destroy(rule_queue, theProgram->num_rules_next_uint());
      assert(rule_queue.empty(theProgram->num_rules_next_uint()));
   }
   if(match_counter) {
      //cout << "==================================\n";
      //match_counter->print(theProgram->get_total_arguments());
      delete_counter(match_counter, theProgram->get_total_arguments());
   }
   for(auto it(allocated_match_objects.begin()), end(allocated_match_objects.end()); it != end; ++it)
      mem::allocator<utils::byte>().deallocate(*it, MATCH_OBJECT_SIZE * All->NUM_THREADS);
}

}
