
#ifndef QUEUE_SAFE_SIMPLE_PQUEUE_HPP
#define QUEUE_SAFE_SIMPLE_PQUEUE_HPP

#include <iostream>

#include "queue/heap_implementation.hpp"

namespace queue
{

template <class T>
class heap_queue
{
private:
	
	class heap_object
	{
	public:
		int val;
		T data;
	};
	
	HEAP_DEFINE_UTILS;
	
#define HEAP_GET_PRIORITY(OBJ) (OBJ).val

	HEAP_DEFINE_HEAPIFYUP;
	
	HEAP_DEFINE_HEAPIFYDOWN;
	
	HEAP_DEFINE_DATA;

public:
	
	HEAP_DEFINE_INVALID_PRIORITY;
	
	HEAP_DEFINE_EMPTY;
	
	void insert(T el, const int prio)
	{
		heap_object obj;
		
		obj.data = el;
		obj.val = prio;
		
		heap.push_back(obj);
		heapifyup(heap.size() - 1);
	}
	
	HEAP_DEFINE_MIN_VALUE;
	
	T pop(void)
	{
		const heap_object min(heap.front());
		
		heap[0] = heap.at(heap.size() - 1);
		heap.pop_back();
		heapifydown(0);
		
		return min.data;
	}
	
	HEAP_DEFINE_PRINT;
	
	explicit heap_queue(void) {}
	~heap_queue(void) {}
};

}

#endif

