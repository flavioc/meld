
#ifndef QUEUE_SAFE_COMPLEX_PQUEUE_HPP
#define QUEUE_SAFE_COMPLEX_PQUEUE_HPP

#include "utils/mutex.hpp"
#include "queue/intrusive_implementation.hpp"
#include "queue/heap_implementation.hpp"

namespace queue
{
	
template <class T>
class intrusive_safe_complex_pqueue
{
private:
	
	typedef T* heap_object;

	mutable utils::mutex mtx;

   static const bool debug = false;

#define HEAP_GET_PRIORITY(OBJ) (__INTRUSIVE_PRIORITY(OBJ))
#define HEAP_GET_POS(OBJ) __INTRUSIVE_POS(OBJ)
#define HEAP_COMPARE(V1, V2) (typ == HEAP_ASC ? ((V1) <= (V2)) : ((V1) >= (V2)))

	HEAP_DEFINE_UTILS;
	
	HEAP_DEFINE_HEAPIFYUP;
	
	HEAP_DEFINE_HEAPIFYDOWN;
	
	HEAP_DEFINE_DATA;
	
	inline void do_insert(heap_object node, const double prio)
	{
		__INTRUSIVE_PRIORITY(node) = prio;
		__INTRUSIVE_QUEUE(node) = queue_number;
		
		heap.push_back(node);
		HEAP_GET_POS(node) = heap.size() - 1;
		heapifyup(heap.size() - 1);
	}
	
	heap_object do_pop(const queue_id_t new_state = queue_no_queue)
	{
		if(empty())
			return NULL;

		const heap_object min(heap.front());
		
      if(heap.size() > 1) {
         heap[0] = heap.back();
         HEAP_GET_POS(heap[0]) = 0;
         heap.pop_back();
         heapifydown(0);
      } else {
         heap.pop_back();
      }
		
      assert(__INTRUSIVE_QUEUE(min) == queue_number);
		__INTRUSIVE_QUEUE(min) = new_state;
		
		return min;
	}

	void do_remove(heap_object obj, const queue_id_t new_state = queue_no_queue)
	{
		size_t index(__INTRUSIVE_POS(obj));
		heap_object tmp(heap.back());

		heap.pop_back();
		
		__INTRUSIVE_QUEUE(obj) = new_state;
		
		if(!heap.empty() && index != heap.size()) {
			HEAP_SET_INDEX(index, tmp);

			const int p(parent(index));

			if(p != -1) {
				if(HEAP_COMPARE(HEAP_GET_PRIORITY(tmp), HEAP_GET_PRIORITY(heap[p])))
					heapifyup(index);
				else
					heapifydown(index);
			} else {
				heapifydown(index);
			}
		}
	}

public:
	
	HEAP_DEFINE_EMPTY;
   HEAP_DEFINE_SIZE;
   HEAP_DEFINE_IN_HEAP;
	
	inline void insert(heap_object node, const double prio)
	{
      utils::lock_guard l(mtx);
      LOCK_STAT(ready_lock);
		do_insert(node, prio);
	}

   inline void start_initial_insert(const size_t many)
   {
      heap.resize(many);
   }

   inline void initial_fast_insert(heap_object node, const double prio, const size_t i)
   {
      assert(__INTRUSIVE_QUEUE(node) != queue_number);
		__INTRUSIVE_PRIORITY(node) = prio;
		__INTRUSIVE_QUEUE(node) = queue_number;
		
      heap[i] = node;
		HEAP_GET_POS(node) = i;
   }
	
	inline double min_value(void) const
	{
      utils::lock_guard l(mtx);
      LOCK_STAT(ready_lock);

      if(empty())
         return 0.0;

		return __INTRUSIVE_PRIORITY(heap.front());
	}
	
	inline heap_object pop(const queue_id_t new_state)
	{
      utils::lock_guard l(mtx);
      LOCK_STAT(ready_lock);
		return do_pop(new_state);
	}

   inline size_t pop_half(heap_object *buffer, const size_t max, const queue_id_t new_state)
   {
      utils::lock_guard l(mtx);
      LOCK_STAT(ready_lock);

      const size_t half(std::min(max, heap.size()/2));
      size_t got(0);
      if(got == half)
         return half;

      size_t level(2);
      for(size_t i(1); i < heap.size(); ) {
         for(size_t j(0); j < level/2; ++j) {
            heap_object obj(heap[i + j]);
            do_remove(obj, new_state);
            buffer[got++] = obj;
            if(got == half)
               return half;
         }
         i += level;
         level <<= 1;
      }

      return got;
   }

   inline heap_object pop_best(intrusive_safe_complex_pqueue<T>& other, const queue_id_t new_state)
   {
      utils::lock_guard l1(mtx);
      utils::lock_guard l2(other.mtx);
      LOCK_STAT(ready_lock);
      LOCK_STAT(ready_lock);

      if(empty()) {
         if(other.empty())
            return NULL;
         return other.do_pop(new_state);
      } else {
         if(other.empty())
            return do_pop(new_state);
         if(HEAP_COMPARE(HEAP_GET_PRIORITY(heap.front()), HEAP_GET_PRIORITY(other.heap.front())))
            return do_pop(new_state);
         else
            return other.do_pop(new_state);
      }
   }

	inline bool remove(heap_object obj, const queue_id_t new_state)
	{
      utils::lock_guard l(mtx);
      LOCK_STAT(ready_lock);
      if(__INTRUSIVE_QUEUE(obj) != queue_number)
         return false;
		do_remove(obj, new_state);
      return true;
	}
	
	void move_node(heap_object node, const double new_prio)
	{
      utils::lock_guard l(mtx);
      LOCK_STAT(ready_lock);
      if(__INTRUSIVE_QUEUE(node) != queue_number)
         return; // not in the queue
		do_remove(node, queue_number);
		do_insert(node, new_prio);
	}

	void set_type(const heap_type _typ)
	{
		typ = _typ;
	}

	void assert_heap(void)
	{
		for(size_t i(0); i < heap.size(); ++i) {
			const int l(left(i));
			const int r(right(i));

			if(l != -1) {
				assert(HEAP_COMPARE(HEAP_GET_PRIORITY(heap[i]), HEAP_GET_PRIORITY(heap[l])));
			}
			if(r != -1) {
				assert(HEAP_COMPARE(HEAP_GET_PRIORITY(heap[i]), HEAP_GET_PRIORITY(heap[r])));
			}
		}
	}

	void print(std::ostream& out)
	{
		for(typename heap_vector::iterator it(heap.begin()),
											end(heap.end());
			it != end;
			++it)
		{
			heap_object obj(*it);
			out << HEAP_GET_PRIORITY(obj) << " ";
		}
	}

	intrusive_safe_complex_pqueue(const queue_id_t id): queue_number(id) {}

	intrusive_safe_complex_pqueue(const queue_id_t id,
         const heap_type _typ): queue_number(id), typ(_typ)
   {
   }
	
	~intrusive_safe_complex_pqueue(void)
   {
	}
};

#undef HEAP_GET_POS
#undef HEAP_GET_PRIORITY
#undef HEAP_COMPARE

}

#endif
