
#ifndef QUEUE_SAFE_COMPLEX_PQUEUE_HPP
#define QUEUE_SAFE_COMPLEX_PQUEUE_HPP

#include <iostream>

#include "utils/mutex.hpp"
#include "queue/intrusive_implementation.hpp"
#include "queue/heap_implementation.hpp"

namespace queue {

template <class T>
class intrusive_safe_complex_pqueue {
   public:
   typedef struct {
      T* data;
      vm::priority_t prio;
   } heap_object;

   mutable utils::mutex mtx;

   static const bool debug = false;

#define HEAP_GET_DATA(OBJ) ((OBJ).data)
#define HEAP_GET_PRIORITY(OBJ) ((OBJ).prio)
#define HEAP_SET_POS(OBJ, IDX) __INTRUSIVE_POS(OBJ) = (IDX)
#define HEAP_COMPARE(V1, V2) (typ == HEAP_ASC ? ((V1) <= (V2)) : ((V1) >= (V2)))

   HEAP_DEFINE_UTILS;

   HEAP_DEFINE_HEAPIFYUP;

   HEAP_DEFINE_HEAPIFYDOWN;

   HEAP_DEFINE_DATA;

   inline void do_insert(T* node, const vm::priority_t prio) {
      heap_object obj{node, prio};
      __INTRUSIVE_QUEUE(node) = queue_number;

      heap.push_back(obj);
      HEAP_SET_POS(node, heap.size() - 1);
      heapifyup(heap.size() - 1);
   }

   inline T* do_pop(const queue_id_t new_state = queue_no_queue) {
      if (empty()) return nullptr;
      //      std::cout << heap.size() << " " << heap.capacity() << "\n";

      const heap_object min(heap.front());

      if (heap.size() > 1) {
         heap[0] = heap.back();
         HEAP_SET_POS(HEAP_GET_DATA(heap[0]), 0);
         heap.pop_back();
         heapifydown(0);
      } else {
         heap.pop_back();
         LOG_HEAP_OPERATION();
      }

      assert(__INTRUSIVE_QUEUE(HEAP_GET_DATA(min)) == queue_number);
      __INTRUSIVE_QUEUE(HEAP_GET_DATA(min)) = new_state;

      return min.data;
   }

   inline void do_remove(T* node, const queue_id_t new_state = queue_no_queue) {
      size_t index(__INTRUSIVE_POS(node));
      heap_object tmp(heap.back());

      heap.pop_back();

      __INTRUSIVE_QUEUE(node) = new_state;

      if (!heap.empty() && index != heap.size()) {
         HEAP_SET_INDEX(index, tmp);

         const int p(parent(index));

         if (p != -1) {
            if (HEAP_COMPARE(HEAP_GET_PRIORITY(tmp),
                             HEAP_GET_PRIORITY(heap[p])))
               heapifyup(index);
            else
               heapifydown(index);
         } else {
            heapifydown(index);
         }
      } else {
         LOG_HEAP_OPERATION();
      }
   }

   public:
   HEAP_DEFINE_EMPTY;
   HEAP_DEFINE_SIZE;
   HEAP_DEFINE_IN_HEAP;

   inline void insert(T* node, const vm::priority_t prio LOCKING_STAT_FLAG) {
      MUTEX_LOCK_GUARD_FLAG(mtx, priority_lock, coord_priority_lock);
      do_insert(node, prio);
   }

   inline void start_initial_insert(const size_t many) { heap.resize(many); }

   inline void initial_fast_insert(T* node, const vm::priority_t prio,
                                   const size_t i) {
      assert(__INTRUSIVE_QUEUE(node) != queue_number);
      __INTRUSIVE_QUEUE(node) = queue_number;

      HEAP_GET_DATA(heap[i]) = node;
      HEAP_GET_PRIORITY(heap[i]) = prio;
      HEAP_SET_POS(node, i);
   }

   inline vm::priority_t min_value(void) const {
      MUTEX_LOCK_GUARD(mtx, priority_lock);

      if (empty()) return 0.0;

      return HEAP_GET_PRIORITY(heap.front());
   }

   inline T* pop(const queue_id_t new_state) {
      MUTEX_LOCK_GUARD(mtx, priority_lock);
      return do_pop(new_state);
   }

   inline bool do_pop_half(T** buffer, const size_t max,
         const queue_id_t new_state)
   {
      const size_t half(std::min(max, heap.size() / 2));
      size_t got(0);
      if (got == half) return half;

      size_t level(2);
      for (size_t i(1); i < heap.size();) {
         for (size_t j(0); j < level / 2; ++j) {
            const size_t idx = std::min(heap.size() - 1, i + j);
            heap_object obj(heap[idx]);
            do_remove(HEAP_GET_DATA(obj), new_state);
            buffer[got++] = obj.data;
            if (got == half) return half;
         }
         i += level;
         level <<= 1;
      }

      return got;
   }

   inline size_t pop_half(T** buffer, const size_t max,
                          const queue_id_t new_state) {
      MUTEX_LOCK_GUARD(mtx, priority_lock);

      return do_pop_half(buffer, max, new_state);
   }

   inline T* pop_best(intrusive_safe_complex_pqueue<T>& other,
                      const queue_id_t new_state) {
      MUTEX_LOCK_GUARD_NAME(l1, mtx, priority_lock);
      MUTEX_LOCK_GUARD_NAME(l2, other.mtx, priority_lock);

      if (empty()) {
         if (other.empty()) return nullptr;
         return other.do_pop(new_state);
      } else {
         if (other.empty()) return do_pop(new_state);
         if (HEAP_COMPARE(HEAP_GET_PRIORITY(heap.front()),
                          HEAP_GET_PRIORITY(other.heap.front())))
            return do_pop(new_state);
         else
            return other.do_pop(new_state);
      }
   }

   inline bool remove(T* node, const queue_id_t new_state LOCKING_STAT_FLAG) {
      MUTEX_LOCK_GUARD_FLAG(mtx, priority_lock, coord_priority_lock);
      if (heap.empty()) return false;
      if (__INTRUSIVE_QUEUE(node) != queue_number) return false;
      do_remove(node, new_state);
      return true;
   }

   inline void do_move_node(T* node, const vm::priority_t new_prio) {
      if (__INTRUSIVE_QUEUE(node) != queue_number) return;  // not in the queue
      const size_t pos(__INTRUSIVE_POS(node));
      const vm::priority_t old_prio(HEAP_GET_PRIORITY(heap[pos]));
      HEAP_GET_PRIORITY(heap[pos]) = new_prio;
      if(compare(old_prio, new_prio)) // asc old_prio <= new_prio
         heapifydown(pos);
      else
         heapifyup(pos);
#if 0
      do_remove(node, queue_number);
      do_insert(node, new_prio);
#endif
   }

   inline void move_node(T* node,
                         const vm::priority_t new_prio LOCKING_STAT_FLAG) {
      MUTEX_LOCK_GUARD_FLAG(mtx, priority_lock, coord_priority_lock);
      if (heap.empty()) return;
      do_move_node(node, new_prio);
   }

   void set_type(const heap_type _typ) { typ = _typ; }

   void assert_heap(void) {
      for (size_t i(0); i < heap.size(); ++i) {
         const int l(left(i));
         const int r(right(i));

         if (l != -1) {
            assert(HEAP_COMPARE(HEAP_GET_PRIORITY(heap[i]),
                                HEAP_GET_PRIORITY(heap[l])));
         }
         if (r != -1) {
            assert(HEAP_COMPARE(HEAP_GET_PRIORITY(heap[i]),
                                HEAP_GET_PRIORITY(heap[r])));
         }
      }
   }

   void print(std::ostream& out) {
      for (typename heap_vector::iterator it(heap.begin()), end(heap.end());
           it != end; ++it) {
         heap_object obj(*it);
         out << HEAP_GET_PRIORITY(obj) << " ";
      }
   }

   inline bool compare(const vm::priority_t v1, const vm::priority_t v2)
   {
      return HEAP_COMPARE(v1, v2);
   }

   intrusive_safe_complex_pqueue(const queue_id_t id) : queue_number(id) {}

   intrusive_safe_complex_pqueue(const queue_id_t id, const heap_type _typ)
       : queue_number(id), typ(_typ) {}

   ~intrusive_safe_complex_pqueue(void) {}
};

#undef HEAP_GET_DATA
#undef HEAP_SET_POS
#undef HEAP_GET_PRIORITY
#undef HEAP_COMPARE
}

#endif
