
#ifndef QUEUE_HEAP_IMPLEMENTATION_HPP
#define QUEUE_HEAP_IMPLEMENTATION_HPP

#include "vm/defs.hpp"

typedef enum {
   HEAP_ASC,
   HEAP_DESC
} heap_type;

#define HEAP_DEFINE_DATA \
	typedef std::vector<heap_object, mem::allocator<heap_object> > heap_vector;	\
   queue_id_t queue_number;                                                \
   heap_type typ;                                                                \
	heap_vector heap
	
#define HEAP_DEFINE_EMPTY				\
	inline bool empty(void) const		\
	{											\
		return heap.empty();				\
	}

#define HEAP_DEFINE_SIZE            \
   inline size_t size(void) const   \
   {                                \
      return heap.size();           \
   }

#define HEAP_DEFINE_IN_HEAP                           \
   inline bool in_queue(heap_object obj) {            \
      return __INTRUSIVE_QUEUE(obj) == queue_number;  \
   }
	
#define HEAP_DEFINE_UTILS								\
	inline int left(const int parent) const		\
	{															\
		int i = (parent << 1) + 1;						\
		return (i < (int)heap.size()) ? i : -1;	\
	}															\
																\
	inline int right(const int parent) const		\
	{															\
		int i = (parent << 1) + 2;						\
		return (i < (int)heap.size()) ? i : -1;	\
	}															\
																\
	inline int parent(const int child) const		\
	{															\
		if(child != 0)										\
			return (child - 1) >> 1;					\
		return -1;											\
	}
	
#define HEAP_DEFINE_PRINT												\
	void print(std::ostream& out)										\
	{																			\
		for(typename heap_vector::iterator it(heap.begin()), 	\
											end(heap.end());				\
			it != end;														\
			++it)																\
		{																		\
			out << it->data << " (" << it->val << ") ";			\
		}																		\
	}

#define HEAP_DEFINE_HEAPIFYUP											\
	bool heapifyup(int index)											\
	{																			\
		bool moved(false);												\
      LOG_HEAP_OPERATION();                                 \
		while((index > 0) && (parent(index) >= 0) &&				\
			(!HEAP_COMPARE(HEAP_GET_PRIORITY(heap[parent(index)]), HEAP_GET_PRIORITY(heap[index]))))	\
		{																		\
			heap_object obj(heap[parent(index)]);					\
																				\
			HEAP_SET_INDEX(parent(index), heap[index]);			\
			HEAP_SET_INDEX(index, obj);								\
         LOG_HEAP_OPERATION();                              \
			index = parent(index);										\
			moved = true;													\
		}																		\
		return moved;														\
	}
	
#define HEAP_DEFINE_MIN_VALUE											\
	double min_value(void) const								      \
	{																			\
		assert(!heap.empty());											\
		return heap.at(0).val;											\
	}
	
#define HEAP_SET_INDEX(IDX, OBJ)	do {								\
	heap[IDX] = OBJ;														\
	HEAP_GET_POS(OBJ) = IDX;											\
} while(false)
	
#define HEAP_DEFINE_HEAPIFYDOWN										\
	void heapifydown(int index)								      \
	{																			\
start:                                                      \
		const int l = left(index);										\
		const int r = right(index);									\
		const bool hasleft = (l >= 0);								\
		const bool hasright = (r >= 0);								\
      LOG_HEAP_OPERATION();                                 \
																				\
		if(!hasleft && !hasright)										\
			return;															\
																				\
		if(hasleft &&														\
HEAP_COMPARE(HEAP_GET_PRIORITY(heap[index]), HEAP_GET_PRIORITY(heap[l]))	\
		&& ((hasright && HEAP_COMPARE(HEAP_GET_PRIORITY(heap[index]),			\
			HEAP_GET_PRIORITY(heap[r])))								\
					|| !hasright))											\
			return;															\
																				\
		int smaller;														\
		if(hasright && HEAP_COMPARE(HEAP_GET_PRIORITY(heap[r]),		\
					HEAP_GET_PRIORITY(heap[l])))						\
			smaller = r;													\
		else																	\
			smaller = l;													\
																				\
		heap_object obj(heap[index]);									\
																				\
		HEAP_SET_INDEX(index, heap[smaller]);						\
		HEAP_SET_INDEX(smaller, obj);								   \
      index = smaller; goto start;                          \
	}

#endif
