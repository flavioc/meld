
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
#include "mem/allocator.hpp"
#include "db/trie.hpp"
#include "vm/defs.hpp"
#include "vm/match.hpp"
#include "utils/mutex.hpp"
#include "vm/rule_matcher.hpp"
#include "vm/all.hpp"
#include "db/linear_store.hpp"
#include "db/temporary_store.hpp"
#include "vm/priority.hpp"
#include "queue/intrusive.hpp"
#include "db/persistent_store.hpp"
#include "mem/node.hpp"
#include "vm/buffer_node.hpp"

namespace sched {
class thread;
}

namespace db {

struct node {
   public:
   std::atomic<vm::ref_count> refs{0};

   typedef vm::node_val node_id;

   private:
   node_id id;
   node_id translation;

   public:
   DECLARE_DOUBLE_QUEUE_NODE(node);

   mem::node_allocator alloc;

   // for managing dynamically allocated nodes.
   node *dyn_next{nullptr}, *dyn_prev{nullptr};
   std::atomic<void *> creator{nullptr};

   private:
   sched::thread *owner{nullptr};

   vm::priority_t default_priority_level;
   vm::priority_t priority_level;

   public:
   inline node_id get_id(void) const { return id; }
   inline node_id get_translated_id(void) const { return translation; }

   inline void set_owner(sched::thread *_owner) { owner = _owner; }
   inline sched::thread *get_owner(void) const { return owner; }

   void assert_end(void) const;

   size_t count_total(const vm::predicate *) const;
   size_t count_total_all(void) const;
   inline bool garbage_collect(void) const {
      return refs == 0 && matcher.is_empty() && !unprocessed_facts;
   }

   inline bool try_garbage_collect() {
      return --refs == 0 && garbage_collect();
   }

   inline vm::priority_t get_default_priority() const {
      return default_priority_level;
   }

   inline vm::priority_t get_priority(void) const {
      if (priority_level == vm::no_priority_value())
         return default_priority_level;
      return priority_level;
   }

   inline void set_temporary_priority(const vm::priority_t level) {
      priority_level = level;
   }
   inline bool set_temporary_priority_if(const vm::priority_t level) {
      if (vm::higher_priority(level, get_priority())) {
         priority_level = level;
         return true;
      }
      return false;
   }
   inline void remove_temporary_priority(void) {
      priority_level = default_priority_level;
   }
   inline void set_default_priority(const vm::priority_t level) {
      default_priority_level = level;
   }
   inline bool has_priority(void) const {
      return get_priority() != vm::no_priority_value();
   }

   inline sched::thread *get_static(void) const { return static_node; }
   inline void set_static(sched::thread *b) { static_node = b; }
   inline void set_moving(void) { static_node = nullptr; }
   inline bool is_static(void) const { return static_node != nullptr; }
   inline bool is_moving(void) const { return static_node == nullptr; }
   inline bool has_new_owner(void) const {
      if (static_node) return owner != static_node;
      return false;
   }

   void print(std::ostream &) const;
   void dump(std::ostream &) const;

   void just_moved();
   void just_moved_buffer();

   // internal databases of the node.
   persistent_store pers_store;
   linear_store linear;
   temporary_store store;
   vm::rule_matcher matcher;
   bool unprocessed_facts{false};
   // marker that indicates if the node should not be stolen.
   // when not nullptr it indicates which scheduler it needs to be on.
   sched::thread *static_node = nullptr;
   utils::mutex main_lock;
   utils::mutex database_lock;

   uint16_t rounds = 0;
#ifdef DYNAMIC_INDEXING
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

   inline void make_inactive(void) { __INTRUSIVE_QUEUE(this) = queue_no_queue; }

   inline void add_linear_fact(vm::tuple *tpl, vm::predicate *pred)
       __attribute__((always_inline)) {
      matcher.new_linear_fact(pred->get_id());
      linear.add_fact(tpl, pred, &alloc);
   }

   inline void add_persistent_fact(
       vm::tuple *tpl, vm::predicate *pred,
       const vm::derivation_direction dir = vm::POSITIVE_DERIVATION,
       const vm::depth_t depth = 0) {
      auto stpl(new vm::full_tuple(tpl, pred, dir, depth));
      store.incoming_persistent_tuples.push_back(stpl);
   }

   inline void inner_add_work_myself(
       vm::tuple *tpl, vm::predicate *pred,
       const vm::derivation_direction dir = vm::POSITIVE_DERIVATION,
       const vm::depth_t depth = 0) {
      if (pred->is_action_pred())
         store.incoming_action_tuples.push_back(
             new vm::full_tuple(tpl, pred, dir, depth));
      else if (pred->is_persistent_pred() || pred->is_reused_pred())
         add_persistent_fact(tpl, pred, dir, depth);
      else
         add_linear_fact(tpl, pred);
   }

   inline void add_work_myself(
       vm::tuple *tpl, vm::predicate *pred,
       const vm::derivation_direction dir = vm::POSITIVE_DERIVATION,
       const vm::depth_t depth = 0) __attribute__((always_inline)) {
      unprocessed_facts = true;

      inner_add_work_myself(tpl, pred, dir, depth);
   }

   inline void add_work_myself(vm::buffer_node &b)
       __attribute__((always_inline)) {
      unprocessed_facts = true;
      for (auto i : b.ls) {
         vm::predicate *pred(i.pred);
         if (pred->is_action_pred()) {
            assert(false);
         } else if (pred->is_persistent_pred() || pred->is_reused_pred()) {
            for (auto it(i.ls.begin()), end(i.ls.end()); it != end;) {
               vm::tuple *tpl(*it);
               ++it;
               auto stpl(
                   new vm::full_tuple(tpl, pred, vm::POSITIVE_DERIVATION, 0));
               store.incoming_persistent_tuples.push_back(stpl);
            }
         } else {
            matcher.new_linear_fact(pred->get_id());
            linear.add_fact_list(i.ls, pred, &alloc);
         }
      }
   }

   inline void add_work_myself(vm::full_tuple_list &ls) {
      unprocessed_facts = true;
      for (auto it(ls.begin()), end(ls.end()); it != end;) {
         vm::full_tuple *x(*it);
         vm::predicate *pred(x->get_predicate());
         // need to be careful to not mangle the pointer
         it++;

         if (pred->is_action_pred())
            store.incoming_action_tuples.push_back(x);
         else if (pred->is_persistent_pred() || pred->is_reused_pred()) {
            store.incoming_persistent_tuples.push_back(x);
         } else {
            add_linear_fact(x->get_tuple(), pred);
            delete x;
         }
      }
   }

   inline void inner_add_work_others(
       vm::tuple *tpl, const vm::predicate *pred,
       const vm::derivation_direction dir = vm::POSITIVE_DERIVATION,
       const vm::depth_t depth = 0) {
      if (pred->is_action_pred()) {
         auto stpl(new vm::full_tuple(tpl, pred, dir, depth));
         store.incoming_action_tuples.push_back(stpl);
      } else if (pred->is_persistent_pred() || pred->is_reused_pred()) {
         auto stpl(new vm::full_tuple(tpl, pred, dir, depth));
         store.incoming_persistent_tuples.push_back(stpl);
      } else
         store.add_incoming(tpl, pred);
   }

   inline void add_work_others(
       vm::tuple *tpl, const vm::predicate *pred,
       const vm::derivation_direction dir = vm::POSITIVE_DERIVATION,
       const vm::depth_t depth = 0) {
      unprocessed_facts = true;

      inner_add_work_others(tpl, pred, dir, depth);
   }

   inline void add_work_others(vm::buffer_node &b) {
      unprocessed_facts = true;
      for (auto i : b.ls) {
         vm::predicate *pred(i.pred);
         vm::tuple_list &ls(i.ls);
         if (pred->is_action_pred()) {
            for (auto it(ls.begin()), end(ls.end()); it != end;) {
               vm::tuple *tpl(*it);
               ++it;
               auto stpl(
                   new vm::full_tuple(tpl, pred, vm::POSITIVE_DERIVATION, 0));
               store.incoming_action_tuples.push_back(stpl);
            }
         } else if (pred->is_persistent_pred() || pred->is_reused_pred()) {
            for (auto it(ls.begin()), end(ls.end()); it != end;) {
               vm::tuple *tpl(*it);
               ++it;
               auto stpl(
                   new vm::full_tuple(tpl, pred, vm::POSITIVE_DERIVATION, 0));
               store.incoming_persistent_tuples.push_back(stpl);
            }
         } else {
            store.add_incoming_list(ls, pred);
         }
      }
   }

   inline void manage_index() {
      linear.improve_index();
      rounds++;
      if (rounds > 0 && rounds % 1 == 0) linear.cleanup_index(&alloc);
   }

   inline explicit node(const node_id _id, const node_id _trans)
       : id(_id),
         translation(_trans),
         default_priority_level(vm::theProgram->get_default_priority()),
         priority_level(vm::theProgram->get_initial_priority()) {}

   inline void wipeout(vm::candidate_gc_nodes &gc_nodes,
                       const bool fast = false) {
      linear.destroy(&alloc, gc_nodes, fast);
      pers_store.wipeout(&alloc, gc_nodes);

      mem::allocator<node>().destroy(this);
   }

   inline void deallocate() { mem::allocator<node>().deallocate(this, 1); }

   inline void set_ids(const node_id _id, const node_id _trans) {
      id = _id;
      translation = _trans;
   }

   static node *create(const node_id id, const node_id translate) {
      node *p(mem::allocator<node>().allocate(1));
      mem::allocator<node>().construct(p, id, translate);
      return p;
   }

   ~node() {}
};

std::ostream &operator<<(std::ostream &, const node &);

struct node_hash {
   size_t operator()(const db::node *x) const {
      return std::hash<node::node_id>()(x->get_id());
   }
};
}

#endif
