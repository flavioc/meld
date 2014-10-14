
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
#include "queue/intrusive.hpp"

#ifdef USE_UI
#include <json_spirit.h>
#endif

namespace sched { class base; class mpi_handler; }
namespace process { class process; class machine; }

namespace db {

class node: public mem::base
{
public:
   
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
   
   // code to handle local stratification
   friend class sched::base;
   friend class process::process;
   friend class process::machine;
   
   sched::base *owner;
   
public:

   void assert_tries(void);
   
   inline node_id get_id(void) const { return id; }
   inline node_id get_translated_id(void) const { return translation; }

   inline void set_owner(sched::base *_owner) { owner = _owner; }
   inline sched::base *get_owner(void) const { return owner; }
   
   bool add_tuple(vm::tuple*, vm::predicate *, const vm::derivation_count,
         const vm::depth_t);
   delete_info delete_tuple(vm::tuple *, vm::predicate *,
         const vm::derivation_count, const vm::depth_t);
   
   db::agg_configuration* add_agg_tuple(vm::tuple*, vm::predicate *,
         const vm::derivation_count, const vm::depth_t);
   db::agg_configuration* remove_agg_tuple(vm::tuple*, vm::predicate *,
         const vm::derivation_count, const vm::depth_t);
   vm::full_tuple_list end_iteration(void);
   
   void delete_by_index(vm::predicate*, const vm::match&);
   void delete_by_leaf(vm::predicate*, tuple_trie_leaf*, const vm::depth_t);
   void delete_all(const vm::predicate*);
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const {}
   
   db::tuple_trie::tuple_search_iterator match_predicate(const vm::predicate_id) const;
  	db::tuple_trie::tuple_search_iterator match_predicate(const vm::predicate_id, const vm::match*) const;
   
   size_t count_total(const vm::predicate_id) const;
   size_t count_total_all(void) const;
   
   void print(std::ostream&) const;
   void dump(std::ostream&) const;
#ifdef USE_UI
	json_spirit::Value dump_json(void) const;
#endif

   // internal database of the node.
   // manages rule execution.
   db::linear_store linear;
   vm::temporary_store store;
   bool unprocessed_facts;
   utils::mutex main_mtx;
   utils::mutex internal_mtx;

#ifdef DYNAMIC_INDEXING
   uint16_t rounds;
   vm::deterministic_timestamp indexing_epoch;
#endif

   // overall locking for changing the state of the node
   inline void lock(LOCK_ARGUMENT) { main_mtx.lock(LOCK_ARGUMENT_NAME); }
   inline bool try_lock(LOCK_ARGUMENT) { return main_mtx.try_lock(LOCK_ARGUMENT_NAME); }
   inline void unlock(LOCK_ARGUMENT) { main_mtx.unlock(LOCK_ARGUMENT_NAME); }

   inline void internal_lock(LOCK_ARGUMENT) { internal_mtx.lock(LOCK_ARGUMENT_NAME); }
   inline bool try_internal_lock(LOCK_ARGUMENT) { return internal_mtx.try_lock(LOCK_ARGUMENT_NAME); }
   inline void internal_unlock(LOCK_ARGUMENT) { internal_mtx.unlock(LOCK_ARGUMENT_NAME); }

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
   
   void wipeout(void);

   // destructor does nothing, use wipeout if needed.
   virtual ~node(void) {}
};

std::ostream& operator<<(std::ostream&, const node&);

}

#endif

