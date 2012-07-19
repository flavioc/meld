
#ifndef QUEUE_SAFE_COMPLEX_PQUEUE_HPP
#define QUEUE_SAFE_COMPLEX_PQUEUE_HPP

#include "queue/intrusive_implementation.hpp"
#include "queue/heap_implementation.hpp"

namespace queue
{
	
template <class T>
class intrusive_safe_complex_pqueue
{
private:
	
	typedef T* heap_object;

#define HEAP_GET_PRIORITY(OBJ) __INTRUSIVE_PRIORITY(OBJ)
#define HEAP_GET_POS(OBJ) __INTRUSIVE_POS(OBJ)

	HEAP_DEFINE_UTILS;
	
	HEAP_DEFINE_HEAPIFYUP;
	
	HEAP_DEFINE_HEAPIFYDOWN;
	
	HEAP_DEFINE_DATA;

public:
	
	HEAP_DEFINE_EMPTY;
	
	HEAP_DEFINE_INVALID_PRIORITY;
	
	static inline bool in_queue(heap_object node)
	{
   	return __INTRUSIVE_IN_PRIORITY_QUEUE(node);
	}
	
	void insert(heap_object node, const int prio)
	{
		__INTRUSIVE_PRIORITY(node) = prio;
		__INTRUSIVE_IN_PRIORITY_QUEUE(node) = true;
		
		heap.push_back(node);
		HEAP_GET_POS(node) = heap.size() - 1;
		heapifyup(heap.size() - 1);
	}
	
	int min_value(void) const
	{
		if(empty())
			return INVALID_PRIORITY;
			
		return __INTRUSIVE_PRIORITY(heap.front());
	}
	
	heap_object pop(void)
	{
		if(empty())
			return NULL;
		
		const heap_object min(heap.front());
		
		heap[0] = heap.back();
		HEAP_GET_POS(heap[0]) = 0;
		heap.pop_back();
		heapifydown(0);
		
		__INTRUSIVE_IN_PRIORITY_QUEUE(min) = false;
		
		return min;
	}
	
	void remove(heap_object obj)
	{
		size_t index(__INTRUSIVE_POS(obj));
		heap_object tmp(heap.back());
		
		heap.pop_back();
		
		__INTRUSIVE_IN_PRIORITY_QUEUE(obj) = false;
		
		if(!heap.empty() && index != heap.size()) {
			HEAP_SET_INDEX(index, tmp);
			
			heapifydown(index);
		}
	}
	
	void move_node(heap_object node, const int new_prio)
	{
		remove(node);
		insert(node, new_prio);
	}
	
	intrusive_safe_complex_pqueue(void) {}
	
	~intrusive_safe_complex_pqueue(void)
   {
      assert(empty());
	}
};

#undef HEAP_GET_POS
#undef HEAP_GET_PRIORITY

}

#endif
