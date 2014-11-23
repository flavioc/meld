
#ifndef NODE_HPP
#define NODE_HPP

#include <atomic>
#include <list>
#include <ostream>
#include <vector>
#include <utility>
#include <unordered_map>

#include "vm/tuple.hpp"
#include "vm/predicate.hpp"
#include "vm/full_tuple.hpp"
#include "db/tuple_aggregate.hpp"
#include "mem/allocator.hpp"
#include "db/trie.hpp"
#include "vm/defs.hpp"
#include "vm/match.hpp"
#include "utils/mutex.hpp"
#include "vm/rule_matcher.hpp"
#include "vm/all.hpp"
#include "db/linear_store.hpp"
#include "vm/temporary.hpp"
#include "vm/priority.hpp"
#include "queue/intrusive.hpp"

#ifdef USE_UI
#include <json_spirit.h>
#endif

namespace sched { class threads_sched; }

namespace db {

struct node
{
public:
	std::atomic<vm::ref_count> refs;

   typedef vm::node_val node_id;
   
   typedef trie::delete_info delete_info;

	DECLARE_DOUBLE_QUEUE_NODE(node);

private:

	node_id id;
   node_id translation;
   
private:
   
   typedef std::unordered_map<vm::predicate_id, tuple_trie*,
               std::hash<vm::predicate_id>,
               std::equal_to<vm::predicate_id>,
               mem::allocator<std::pair<const vm::predicate_id,
                                 tuple_trie*> > > pers_tuple_map;
                                 
   typedef std::unordered_map<vm::predicate_id, tuple_aggregate*,
               std::hash<vm::predicate_id>,
               std::equal_to<vm::predicate_id>,
               mem::allocator<std::pair<const vm::predicate_id,
                                 tuple_aggregate*> > > aggregate_map;
	
	// tuple database
   pers_tuple_map tuples;
   
   // sets of tuple aggregates
   aggregate_map aggs;
   
   tuple_trie* get_storage(vm::predicate*);
   
   sched::threads_sched *owner = NULL;

   // marker that indicates if the node should not be stolen.
   // when not NULL it indicates which scheduler it needs to be on.
   sched::threads_sched *static_node = NULL;
	
   vm::priority_t default_priority_level;
   vm::priority_t priority_level;
   
public:

   void assert_tries(void);
   
   inline node_id get_id(void) const { return id; }
   inline node_id get_translated_id(void) const { return translation; }

   inline void set_owner(sched::threads_sched *_owner) { owner = _owner; }
   inline sched::threads_sched *get_owner(void) const { return owner; }
   
   bool add_tuple(vm::tuple*, vm::predicate *, const vm::derivation_count,
         const vm::depth_t);
   delete_info delete_tuple(vm::tuple *, vm::predicate *,
         const vm::derivation_count, const vm::depth_t);
   
   db::agg_configuration* add_agg_tuple(vm::tuple*, vm::predicate *,
         const vm::derivation_count, const vm::depth_t
#ifdef GC_NODES
         , vm::candidate_gc_nodes&
#endif
         );
   db::agg_configuration* remove_agg_tuple(vm::tuple*, vm::predicate *,
         const vm::derivation_count, const vm::depth_t
#ifdef GC_NODES
         , vm::candidate_gc_nodes&
#endif
         );
   vm::full_tuple_list end_iteration(
#ifdef GC_NODES
         vm::candidate_gc_nodes&
#endif
         );
   
   void delete_by_index(vm::predicate*, const vm::match&
#ifdef GC_NODES
         , vm::candidate_gc_nodes&
#endif
         );
   void delete_by_leaf(vm::predicate*, tuple_trie_leaf*, const vm::depth_t
#ifdef GC_NODES
         , vm::candidate_gc_nodes&
#endif
         );
   void delete_all(const vm::predicate*);
   
   void assert_end(void) const;
   
   db::tuple_trie::tuple_search_iterator match_predicate(const vm::predicate_id) const;
  	db::tuple_trie::tuple_search_iterator match_predicate(const vm::predicate_id, const vm::match*) const;
   
   size_t count_total(const vm::predicate_id) const;
   size_t count_total_all(void) const;
   inline bool garbage_collect(void) const
   {
#ifdef GC_NODES
      return refs == 0 && store.matcher.is_empty() && !unprocessed_facts;
#else
      return false;
#endif
   }

	inline vm::priority_t get_priority(void) const {
      if(priority_level == vm::no_priority_value())
         return default_priority_level;
      return priority_level;
   }

	inline void set_temporary_priority(const vm::priority_t level) { priority_level = level; }
	inline bool set_temporary_priority_if(const vm::priority_t level) {
      if(vm::higher_priority(level, get_priority())) {
         priority_level = level;
         return true;
      }
      return false;
   }
   inline void remove_temporary_priority(void) { priority_level = vm::no_priority_value(); }
   inline void set_default_priority(const vm::priority_t level) { default_priority_level = level; }
	inline bool has_priority(void) const {
      return get_priority() != vm::no_priority_value();
   }

   inline sched::threads_sched* get_static(void) const { return static_node; }
   inline void set_static(sched::threads_sched *b) { static_node = b; }
   inline void set_moving(void) { static_node = NULL; }
   inline bool is_static(void) const { return static_node != NULL; }
   inline bool is_moving(void) const { return static_node == NULL; }
   inline bool has_new_owner(void) const {
      if(static_node)
         return owner != static_node;
      return false;
   }

   void print(std::ostream&) const;
   void dump(std::ostream&) const;
#ifdef USE_UI
	json_spirit::Value dump_json(void) const;
#endif

   // internal database of the node.
   // manages rule execution.
   db::linear_store linear;
   vm::temporary_store store;
   bool unprocessed_facts = false;
   utils::mutex main_lock;
   utils::mutex database_lock;

#ifdef DYNAMIC_INDEXING
   uint16_t rounds = 0;
   vm::deterministic_timestamp indexing_epoch = 0;
#endif

   // return queue of the node.
   inline queue_id_t node_state(void) const {
      // node state is represented by the id of the queue.
      return __INTRUSIVE_QUEUE(this);
   }

   inline bool active_node(void) const {
      return node_state() != queue_no_queue;
   }

   inline void make_inactive(void) {
      __INTRUSIVE_QUEUE(this) = queue_no_queue;
   }

   inline void add_linear_fact(vm::tuple *tpl, vm::predicate *pred)
   {
      store.register_tuple_fact(pred, 1);
      linear.add_fact(tpl, pred, store.matcher);
   }

   inline void inner_add_work_myself(vm::tuple *tpl, vm::predicate *pred,
         const vm::ref_count count, const vm::depth_t depth)
   {
      if(pred->is_action_pred())
         store.add_action_fact(new vm::full_tuple(tpl, pred, count, depth));
      else if(pred->is_persistent_pred() || pred->is_reused_pred()) {
         vm::full_tuple *stpl(new vm::full_tuple(tpl, pred, count, depth));
         store.add_persistent_fact(stpl);
         store.register_fact(stpl);
      } else
         add_linear_fact(tpl, pred);
   }

   inline void add_work_myself(vm::tuple *tpl, vm::predicate *pred,
         const vm::ref_count count, const vm::depth_t depth)
   {
      unprocessed_facts = true;

      inner_add_work_myself(tpl, pred, count, depth);
   }

   inline void add_work_myself(vm::tuple_array& arr)
   {
      unprocessed_facts = true;
      for(auto it(arr.begin()), end(arr.end()); it != end; ++it) {
         vm::full_tuple info(*it);
         inner_add_work_myself(info.get_tuple(), info.get_predicate(), info.get_count(), info.get_depth());
      }
   }

   inline void add_work_myself(vm::full_tuple_list& ls)
   {
      unprocessed_facts = true;
      for(auto it(ls.begin()), end(ls.end()); it != end;) {
         vm::full_tuple *x(*it);
         vm::predicate *pred(x->get_predicate());
         // need to be careful to not mangle the pointer
         it++;

         if(pred->is_action_pred())
            store.add_action_fact(x);
         else if(pred->is_persistent_pred() || pred->is_reused_pred()) {
            store.add_persistent_fact(x);
            store.register_fact(x);
         } else {
            add_linear_fact(x->get_tuple(), pred);
            delete x;
         }
      }
   }

   inline void inner_add_work_others(vm::tuple *tpl, vm::predicate *pred,
         const vm::ref_count count, const vm::depth_t depth)
   {
      if(pred->is_action_pred()) {
         vm::full_tuple *stpl(new vm::full_tuple(tpl, pred, count, depth));
         store.incoming_action_tuples.push_back(stpl);
      } else if(pred->is_persistent_pred() || pred->is_reused_pred()) {
         vm::full_tuple *stpl(new vm::full_tuple(tpl, pred, count, depth));
         store.incoming_persistent_tuples.push_back(stpl);
      } else
         store.add_incoming(tpl, pred);
   }

   inline void add_work_others(vm::tuple *tpl, vm::predicate *pred,
         const vm::ref_count count, const vm::depth_t depth)
   {
      unprocessed_facts = true;

      inner_add_work_others(tpl, pred, count, depth);
   }

   inline void add_work_others(vm::tuple_array& arr)
   {
      unprocessed_facts = true;
      for(auto it(arr.begin()), end(arr.end()); it != end; ++it) {
         vm::full_tuple info(*it);
         inner_add_work_others(info.get_tuple(), info.get_predicate(), info.get_count(), info.get_depth());
      }
   }
   
   explicit node(const node_id, const node_id);
   
#ifdef GC_NODES
   void wipeout(vm::candidate_gc_nodes&);
#else
   void wipeout(void);
#endif

   static node *create(const node_id id, const node_id translate) {
      node *p(mem::allocator<node>().allocate(1));
      mem::allocator<node>().construct(p, id, translate);
      return p;
   }
};

std::ostream& operator<<(std::ostream&, const node&);

}

#endif

