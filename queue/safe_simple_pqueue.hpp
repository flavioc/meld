
#ifndef QUEUE_SAFE_SIMPLE_PQUEUE_HPP
#define QUEUE_SAFE_SIMPLE_PQUEUE_HPP

#include <iostream>

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
	
	inline int left(const int parent) const
	{
		int i = (parent << 1) + 1;
		return (i < heap.size()) ? i : -1;
	}
	
	inline int right(const int parent) const
	{
		int i = (parent << 1) + 2;
		return (i < heap.size()) ? i : -1;
	}
	
	inline int parent(const int child) const
	{
		if(child != 0)
			return (child - 1) >> 1;
		return -1;
	}
	
	void heapifyup(int index)
	{
		while((index > 0) && (parent(index) >= 0) &&
			(heap[parent(index)].val > heap[index].val))
		{
			heap_object obj(heap[parent(index)]);
			
			heap[parent(index)] = heap[index];
			heap[index] = obj;
			index = parent(index);
		}
	}
	
	void heapifydown(const int index)
	{
		int l = left(index);
		int r = right(index);
		const bool hasleft = l >= 0;
		const bool hasright = r >= 0;
		
		if(!hasleft && !hasright)
			return;
			
		if(hasleft && heap[index].val <= heap[l].val
			&& ((hasright && heap[index].val <= heap[r].val)
					|| !hasright))
			return;
		
		int smaller;
		if(hasright && heap[r].val <= heap[l].val)
			smaller = r;
		else
			smaller = l;
		
		const heap_object obj(heap[index]);
			
		heap[index] = heap[smaller];
		heap[smaller] = obj;
		heapifydown(smaller);
	}
	
	typedef std::vector<heap_object> heap_vector;
	heap_vector heap;

public:
	
	inline bool empty(void) const
	{
		return heap.empty();
	}
	
	void insert(T el, const int prio)
	{
		heap_object obj;
		
		obj.data = el;
		obj.val = prio;
		
		heap.push_back(obj);
		heapifyup(heap.size() - 1);
	}
	
	T pop(void)
	{
		const heap_object min(heap.front());
		
		heap[0] = heap.at(heap.size() - 1);
		heap.pop_back();
		heapifydown(0);
		
		return min.data;
	}
	
	void print(std::ostream& out)
	{
		for(typename heap_vector::iterator it(heap.begin()), end(heap.end());
			it != end;
			++it)
		{
			out << it->data << " (" << it->val << ") ";
		}
	}
	
	explicit heap_queue(void) {}
	~heap_queue(void) {}
};

}

#endif

