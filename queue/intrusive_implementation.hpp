
#ifndef QUEUE_INTRUSIVE_IMPLEMENTATION_HPP
#define QUEUE_INTRUSIVE_IMPLEMENTATION_HPP

#include "conf.hpp"
#include "queue/macros.hpp"
#include "queue/intrusive.hpp"

#define QUEUE_DEFINE_INTRUSIVE_DOUBLE_DATA() \
	typedef T* node_type; 							\
   volatile node_type head;						\
	volatile node_type tail;						\
	QUEUE_DEFINE_TOTAL()

#define QUEUE_DEFINE_INTRUSIVE_CONSTRUCTOR(TYPE) 	\
	explicit TYPE(void): 									\
		head(NULL), tail(NULL), total(0)             \
	{																\
	}

#define QUEUE_DEFINE_INTRUSIVE_IN_QUEUE()						\
	static inline bool in_queue(node_type node)				\
	{																		\
   	return __INTRUSIVE_IN_QUEUE(node);						\
	}

#define QUEUE_DEFINE_INTRUSIVE_DOUBLE_EMPTY() 					\
	inline bool empty(void) const { return head == NULL; }

#define QUEUE_DEFINE_INTRUSIVE_DOUBLE_POP_IF_NOT_EMPTY() 	\
	if(empty())																\
      return false;														\
      																		\
   assert(!empty());														\
   																			\
   if(__INTRUSIVE_NEXT(__INTRUSIVE_NEXT(head)) == NULL)		\
      return false;														\
   																			\
   data = head;															\
   head = __INTRUSIVE_NEXT(head);									\
   __INTRUSIVE_PREV(head) = NULL;									\
   																			\
   __INTRUSIVE_PREV(data) = NULL;									\
   __INTRUSIVE_NEXT(data) = NULL;									\
   assert(__INTRUSIVE_IN_QUEUE(data) == true);              \
   __INTRUSIVE_IN_QUEUE(data) = false;								\
   																			\
	QUEUE_DECREMENT_TOTAL();											\
																				\
   return true

#define QUEUE_DEFINE_INTRUSIVE_DOUBLE_POP()							\
	if(empty())																	\
   	return false;															\
   																				\
   assert(!empty());															\
   																				\
   data = head;																\
   head = __INTRUSIVE_NEXT(head);										\
   if(head)																		\
   	__INTRUSIVE_PREV(head) = NULL;									\
   else																			\
      tail = NULL;															\
      																			\
   assert(__INTRUSIVE_IN_QUEUE(data) == true);                 \
   __INTRUSIVE_IN_QUEUE(data) = false;									\
      																			\
   assert((head && __INTRUSIVE_PREV(head) == NULL) || !head);	\
   assert((head && tail) || (!head && !tail));						\
   																				\
	QUEUE_DECREMENT_TOTAL();												\
																					\
   return true

#define QUEUE_DEFINE_INTRUSIVE_DOUBLE_OPS() 			\
	inline void push_head_node(node_type new_node)	\
	{																\
		QUEUE_INCREMENT_TOTAL();							\
																	\
      assert(__INTRUSIVE_IN_QUEUE(new_node) == false);   \
		__INTRUSIVE_IN_QUEUE(new_node) = true;			\
		__INTRUSIVE_NEXT(new_node) = head;				\
																	\
		if(tail == NULL)										\
			tail = new_node;									\
		if(head != NULL)										\
			__INTRUSIVE_PREV(head) = new_node;			\
																	\
		head = new_node;										\
																	\
		assert(__INTRUSIVE_NEXT(tail) == NULL);		\
		assert(__INTRUSIVE_PREV(head) == NULL);		\
		assert(head == new_node);							\
	}																\
																	\
   inline void push_tail_node(node_type new_node)	\
   {																\
		QUEUE_INCREMENT_TOTAL();							\
																	\
      if(!head)												\
         head = new_node;									\
																	\
      __INTRUSIVE_PREV(new_node) = tail;				\
																	\
      if(tail)													\
         __INTRUSIVE_NEXT(tail) = new_node;			\
      else														\
         tail = new_node;									\
																	\
      assert(__INTRUSIVE_IN_QUEUE(new_node) == false);   \
      __INTRUSIVE_IN_QUEUE(new_node) = true;			\
      __INTRUSIVE_NEXT(new_node) = NULL;				\
      tail = new_node;										\
																	\
      assert(tail == new_node);							\
      assert(__INTRUSIVE_NEXT(tail) == NULL);		\
   }

#define QUEUE_DEFINE_INTRUSIVE_MOVE_UP()		{		\
	if(!in_queue(node))										\
		return;													\
																	\
   node_type prev(__INTRUSIVE_PREV(node));			\
      															\
   if(prev == NULL) {										\
      assert(node == head);								\
      return;													\
   }																\
   																\
   node_type next(__INTRUSIVE_NEXT(node));			\
      															\
   __INTRUSIVE_NEXT(prev) = next;						\
   if(next)														\
    	__INTRUSIVE_PREV(next) = prev;					\
         														\
   if(next == NULL)											\
    	tail = prev;											\
      															\
   assert(tail != NULL);									\
   																\
   __INTRUSIVE_PREV(node) = NULL;						\
	__INTRUSIVE_NEXT(node) = head;						\
   __INTRUSIVE_IN_QUEUE(node) = true;					\
	__INTRUSIVE_PREV(head) = node;						\
   head = node;												\
																	\
   if(tail == NULL)											\
		tail = node;											\
}

#define QUEUE_DEFINE_INTRUSIVE_REMOVE(ITEM)		{	\
	node_type prev(__INTRUSIVE_PREV(ITEM));			\
	node_type next(__INTRUSIVE_NEXT(ITEM));			\
																	\
	if(head == ITEM)											\
		head = next;											\
	if(tail == ITEM)											\
		tail = prev;											\
																	\
	if(prev)														\
		__INTRUSIVE_NEXT(prev) = next;					\
	if(next)														\
		__INTRUSIVE_PREV(next) = prev;					\
																	\
	__INTRUSIVE_NEXT(ITEM) = NULL;						\
	__INTRUSIVE_PREV(ITEM) = NULL;						\
   assert(__INTRUSIVE_IN_QUEUE(ITEM) == true);     \
	__INTRUSIVE_IN_QUEUE(ITEM) = false;					\
	QUEUE_DECREMENT_TOTAL();								\
}

#define QUEUE_DEFINE_INTRUSIVE_CONST_ITERATOR()		\
	class const_iterator										\
	{																\
		private:													\
			node_type cur;										\
		public:													\
			inline T operator*(void)						\
			{														\
				return cur;										\
			}														\
																	\
			inline const_iterator&							\
			operator=(const const_iterator& it)			\
			{														\
				cur = it.cur;									\
				return *this;									\
			}														\
																	\
			inline void operator++(void)					\
			{														\
				assert(cur != NULL);							\
				cur = __INTRUSIVE_NEXT(cur);				\
			}														\
																	\
			inline bool											\
			operator==(const const_iterator& it)		\
			const													\
			{														\
				return it.cur == cur;						\
			}														\
			explicit const_iterator(node_type n) : 	\
				cur(n) {}										\
																	\
			explicit const_iterator(void) : 				\
				cur(NULL) {}									\
	};																\
	inline const_iterator begin(void) const			\
	{ return const_iterator(head); }						\
	inline const_iterator end(void) const				\
	{ return const_iterator(NULL); }

#endif

