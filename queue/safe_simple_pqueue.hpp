
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

	heap_type typ;
	
	class heap_object
	{
	public:
		heap_priority val;
		T data;
		int pos;
	};
	
	HEAP_DEFINE_UTILS;
	
#define HEAP_GET_PRIORITY(OBJ) ((typ == HEAP_INT_ASC || typ == HEAP_INT_DESC) ? (OBJ).val.int_priority : (OBJ).val.float_priority)
#define HEAP_COMPARE(V1, V2) ((typ == HEAP_INT_ASC || typ == HEAP_FLOAT_ASC) ? ((V1) <= (V2)) : ((V1) >= (V2)))
#define HEAP_GET_POS(OBJ) (OBJ).pos

	HEAP_DEFINE_HEAPIFYUP;
	
	HEAP_DEFINE_HEAPIFYDOWN;
	
	HEAP_DEFINE_DATA;

public:
	
	HEAP_DEFINE_EMPTY;
	
	class const_iterator
	{
	private:
		size_t cur_pos;
		mutable bool end;
		const heap_vector *vec;
		
	public:
		
		inline T operator*(void)
		{
			return (*vec)[cur_pos].data;
		}
		
		inline const_iterator&
		operator=(const const_iterator& it)
		{
			end = it.end;
			cur_pos = it.cur_pos;
			return *this;
		}
		
		inline void operator++(void)
		{
			++cur_pos;
		}
		
		inline bool
		operator==(const const_iterator& it) const
		{
			if(it.end)
			{
				if(end)
					return true;
				if(vec->size() == cur_pos)
					end = true;
				return end;
			} else
				return cur_pos == it.cur_pos;
		}
		
		inline bool
		operator!=(const const_iterator& it) const
		{
			return !operator==(it);
		}
		
		explicit const_iterator(const heap_vector* v): cur_pos(0), end(false), vec(v) {}
		
		explicit const_iterator(void): end(true), vec(NULL)
		{}
	};
	
	inline const_iterator begin(void) const
	{ return const_iterator(&heap); }
	inline const_iterator end(void) const
	{ return const_iterator(); }
	
	void insert(T el, const heap_priority prio)
	{
		heap_object obj;
		
		obj.data = el;
		obj.val = prio;
		
		heap.push_back(obj);
		HEAP_GET_POS(obj) = heap.size() - 1;
		heapifyup(heap.size() - 1);
	}
	
	void remove(T val, const heap_priority prio)
	{
		int shift = 1;
		int remain = 1;
		size_t index = 0;
		heap_object obj;
		bool forall = true;
		
		obj.data = val;
		obj.val = prio;
		
		/* look at each tree level and find val
		   to prune search find if in all the nodes y in the level follow the condition prio(x) <= prio(y) holds
		*/ 
		
		while (index < heap.size()) {			
			if(heap[index].data == val) {
				remove_by_index(index);
				return;
			}
			
			if(forall && !HEAP_COMPARE(HEAP_GET_PRIORITY(obj), HEAP_GET_PRIORITY(heap[index]))) {
				forall = false;
			}
			
			remain--;
			
			index = index + 1;
			
			if(remain == 0) {
				if(forall) {
					// not found
					assert(false);
					return;
				}
				
				forall = true;
				shift = shift << 1;
				remain = shift;
			}
		}
		assert(false);
	}
	
	void remove_by_index(const size_t index)
	{
		heap_object obj(heap[index]);
		heap_object tmp(heap.back());
		
		heap.pop_back();
		
		if(!heap.empty() && index != heap.size()) {
			HEAP_SET_INDEX(index, tmp);
			
			heapifydown(index);
		}
	}

   void clear(void)
   {
      heap.clear();
   }
	
	HEAP_DEFINE_MIN_VALUE;
	
	T pop(void)
	{
		const heap_object min(heap.front());
		
		heap[0] = heap.back();
		HEAP_GET_POS(heap[0]) = 0;
		heap.pop_back();
		heapifydown(0);
		
		return min.data;
	}
	
	HEAP_DEFINE_PRINT;

	void set_type(const heap_type _typ)
	{
		typ = _typ;
	}
	
	explicit heap_queue(void) {}
	~heap_queue(void) {}
};

#undef HEAP_GET_POS
#undef HEAP_GET_PRIORITY
#undef HEAP_COMPARE

}

#endif

