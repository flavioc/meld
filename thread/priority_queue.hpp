
#ifndef THREAD_PRIORITY_QUEUE_HPP
#define THREAD_PRIORITY_QUEUE_HPP

#include <iostream>
#include <algorithm>

#include "utils/mutex.hpp"
#include "queue/safe_complex_pqueue.hpp"
#include "queue/safe_double_queue.hpp"
#include "db/node.hpp"
#include "vm/all.hpp"

namespace sched {

struct priority_queue {
   using node_queue = queue::intrusive_safe_double_queue<db::node>;
   using queue_t = queue::intrusive_safe_complex_pqueue<db::node>;
   queue_t main_heap;
   size_t total_size{0};
   bool has_extra{false};
   vm::priority_t max_prio{0};
#define MAX_NODES_IN_HEAP 50000
#define NUMBER_OF_EXTRA_QUEUES 4
   node_queue extra[NUMBER_OF_EXTRA_QUEUES];

   private:
   inline void check_prio(const vm::priority_t new_prio) {
      if (max_prio < new_prio) max_prio = new_prio;
   }

   inline void move_from_extra_to_heap() {
      assert(main_heap.size() <= total_size);
      db::node* n{nullptr};
      if (main_heap.size() >= MAX_NODES_IN_HEAP) return;
      for (size_t i(0); i < NUMBER_OF_EXTRA_QUEUES; ++i) {
         node_queue& q(extra[i]);
         while (!q.empty()) {
            if (q.do_pop_head(n, main_heap.queue_number))
               main_heap.do_insert(n, n->get_priority());
            assert(__INTRUSIVE_EXTRA_ID(n) == i + 1);
            __INTRUSIVE_EXTRA_ID(n) = 0;
            if (main_heap.size() >= MAX_NODES_IN_HEAP) return;
         }
      }
      assert(main_heap.size() <= total_size);
      has_extra = false;
   }

   public:
   inline bool empty() const { return total_size == 0; }
   inline size_t size() const { return total_size; }

   inline vm::priority_t min_value(void) const { return main_heap.min_value(); }

   inline void start_initial_insert(const size_t many) {
      main_heap.start_initial_insert(std::min(many, (size_t)MAX_NODES_IN_HEAP));
      if (many > MAX_NODES_IN_HEAP) has_extra = true;
      total_size = many;
   }

   inline void initial_fast_insert(db::node* node, const vm::priority_t prio,
                                   const size_t i) {
      assert(__INTRUSIVE_QUEUE(node) != main_heap.queue_number);

      if (i >= MAX_NODES_IN_HEAP) {
         extra[0].push_tail_node(node);
         __INTRUSIVE_EXTRA_ID(node) = 1;
      } else {
         __INTRUSIVE_EXTRA_ID(node) = 0;
         main_heap.initial_fast_insert(node, prio, i);
      }
   }

   inline void insert(db::node* node,
                      const vm::priority_t prio LOCKING_STAT_FLAG) {
      MUTEX_LOCK_GUARD_FLAG(main_heap.mtx, priority_lock, coord_priority_lock);
      check_prio(prio);
      total_size++;
      const size_t heap_size(main_heap.size());
      if (prio == vm::max_priority_value() ||
          (heap_size < MAX_NODES_IN_HEAP / 2) ||
          (heap_size < MAX_NODES_IN_HEAP && prio < max_prio / 2)) {
         __INTRUSIVE_EXTRA_ID(node) = 0;
         main_heap.do_insert(node, prio);
      } else {
         const double frac(prio / max_prio);
         const size_t idx(
             std::min((size_t)NUMBER_OF_EXTRA_QUEUES - 1,
                      (size_t)((double)NUMBER_OF_EXTRA_QUEUES * frac)));
         __INTRUSIVE_EXTRA_ID(node) = idx + 1;
         __INTRUSIVE_QUEUE(node) = queue_no_queue;
         extra[idx].push_tail_node(node);
         has_extra = true;
      }
      assert(main_heap.size() <= total_size);
   }

   inline bool should_move_from_extra() const {
      return main_heap.size() < MAX_NODES_IN_HEAP / 8;
   }

   inline db::node* do_pop(const queue_id_t new_state) {
      db::node* ret(main_heap.do_pop(new_state));
      total_size--;
      if (has_extra && should_move_from_extra()) move_from_extra_to_heap();
      assert(main_heap.size() <= total_size);

      return ret;
   }

   inline db::node* pop(const queue_id_t new_state) {
      if (empty()) return nullptr;

      MUTEX_LOCK_GUARD(main_heap.mtx, priority_lock);
      return do_pop(new_state);
   }

   inline bool remove(db::node* node,
                      const queue_id_t new_state LOCKING_STAT_FLAG) {
      MUTEX_LOCK_GUARD_FLAG(main_heap.mtx, priority_lock, coord_priority_lock);
      if (empty()) return false;
      if (__INTRUSIVE_QUEUE(node) != main_heap.queue_number) return false;
      const size_t id(__INTRUSIVE_EXTRA_ID(node));
      if (id == 0) {
         if (main_heap.empty()) return false;
         main_heap.do_remove(node, new_state);
      } else {
         node_queue& ls(extra[id - 1]);
         ls.do_remove(node, new_state);
      }
      total_size--;
      return true;
   }

   inline size_t pop_half(db::node** buffer, const size_t max,
                          const queue_id_t new_state) {
      MUTEX_LOCK_GUARD_FLAG(main_heap.mtx, priority_lock, coord_priority_lock);
      const size_t ret(main_heap.do_pop_half(buffer, max, new_state));
      total_size -= ret;
      if (has_extra && should_move_from_extra()) move_from_extra_to_heap();
      return ret;
   }

   inline db::node* pop_best(priority_queue& other,
                             const queue_id_t new_state) {
      MUTEX_LOCK_GUARD_NAME(l1, main_heap.mtx, priority_lock);
      MUTEX_LOCK_GUARD_NAME(l2, other.main_heap.mtx, priority_lock);

      if (empty()) {
         if (other.empty()) return nullptr;
         return do_pop(new_state);
      } else {
         if (other.empty()) return do_pop(new_state);
         if (other.main_heap.compare(min_value(), other.min_value()))
            return do_pop(new_state);
         else
            return other.do_pop(new_state);
      }
   }

   inline void move_node(db::node* node,
                         const vm::priority_t new_prio LOCKING_STAT_FLAG) {
      MUTEX_LOCK_GUARD_FLAG(main_heap.mtx, priority_lock, coord_priority_lock);
      if (__INTRUSIVE_QUEUE(node) != main_heap.queue_number)
         return;  // not in the queue
      const size_t id(__INTRUSIVE_EXTRA_ID(node));
      check_prio(new_prio);
      if (id == 0) {
         main_heap.do_move_node(node, new_prio);
      } else {
         if (main_heap.compare(new_prio, max_prio / 2) &&
             should_move_from_extra()) {
            // must be in the 20% in order to be inserted into the heap.
            extra[id - 1].do_remove(node, main_heap.queue_number);
            __INTRUSIVE_EXTRA_ID(node) = 0;
            main_heap.do_insert(node, new_prio);
         }
      }
   }

   void set_type(const heap_type _typ) { main_heap.set_type(_typ); }

   explicit priority_queue(const queue_id_t id) : main_heap(id) {
      for (size_t i(0); i < NUMBER_OF_EXTRA_QUEUES; ++i)
         extra[i].queue_number = id;
   }

   explicit priority_queue(const queue_id_t id, const heap_type _typ)
       : priority_queue(id) {
      set_type(_typ);
   }

   ~priority_queue(void) {}
};
}

#endif
