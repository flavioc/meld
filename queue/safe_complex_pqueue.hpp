
#ifndef QUEUE_SAFE_COMPLEX_PQUEUE_HPP
#define QUEUE_SAFE_COMPLEX_PQUEUE_HPP

#include "queue/intrusive_implementation.hpp"
#include "queue/heap_implementation.hpp"
#include "utils/spinlock.hpp"


namespace queue
{
	
template <class T>
class intrusive_safe_complex_pqueue
{
private:
	
	typedef T* heap_object;

	heap_type typ;
   static const bool debug = false;

#define HEAP_GET_PRIORITY(OBJ) ((typ == HEAP_INT_ASC || typ == HEAP_INT_DESC) ? \
					(__INTRUSIVE_PRIORITY(OBJ).int_priority) : (__INTRUSIVE_PRIORITY(OBJ).float_priority))
#define HEAP_GET_POS(OBJ) __INTRUSIVE_POS(OBJ)
#define HEAP_COMPARE(V1, V2) ((typ == HEAP_INT_ASC || typ == HEAP_FLOAT_ASC) ? ((V1) <= (V2)) : ((V1) >= (V2)))

	HEAP_DEFINE_UTILS;
	
	HEAP_DEFINE_HEAPIFYUP;
	
	HEAP_DEFINE_HEAPIFYDOWN;
	
	HEAP_DEFINE_DATA;
	
	void do_insert(heap_object node, const heap_priority prio)
	{
      assert(__INTRUSIVE_IN_PRIORITY_QUEUE(node) == false);
		__INTRUSIVE_PRIORITY(node) = prio;
		__INTRUSIVE_IN_PRIORITY_QUEUE(node) = true;
		
		heap.push_back(node);
		HEAP_GET_POS(node) = heap.size() - 1;
		heapifyup(heap.size() - 1);
	}
	
	heap_object do_pop(void)
	{
		if(empty())
			return NULL;

		const heap_object min(heap.front());
		
		heap[0] = heap.back();
		HEAP_GET_POS(heap[0]) = 0;
		heap.pop_back();
		heapifydown(0);
		
      assert(__INTRUSIVE_IN_PRIORITY_QUEUE(min) == true);
		__INTRUSIVE_IN_PRIORITY_QUEUE(min) = false;
		
		return min;
	}

	void do_remove(heap_object obj)
	{
		size_t index(__INTRUSIVE_POS(obj));
		heap_object tmp(heap.back());

		heap.pop_back();
		
		__INTRUSIVE_IN_PRIORITY_QUEUE(obj) = false;
		
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
	
	static inline bool in_queue(heap_object node)
	{
   	return __INTRUSIVE_IN_PRIORITY_QUEUE(node);
	}
	
	void insert(heap_object node, const heap_priority prio)
	{
		do_insert(node, prio);
	}
	
	heap_priority min_value(void) const
	{
		return __INTRUSIVE_PRIORITY(heap.front());
	}
	
	heap_object pop(void)
	{
		return do_pop();
	}
	
	void remove(heap_object obj)
	{
		do_remove(obj);
	}
	
	void move_node(heap_object node, const heap_priority new_prio)
	{
		do_remove(node);
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

	intrusive_safe_complex_pqueue(void) {}

	intrusive_safe_complex_pqueue(const heap_type _typ): typ(_typ) {}
	
	~intrusive_safe_complex_pqueue(void)
   {
      assert(empty());
	}
};

#undef HEAP_GET_POS
#undef HEAP_GET_PRIORITY
#undef HEAP_COMPARE

}

#endif
