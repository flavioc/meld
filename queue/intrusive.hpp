
#ifndef QUEUE_INTRUSIVE_HPP
#define QUEUE_INTRUSIVE_HPP

#include "conf.hpp"

#define NEXT(ITEM) ((ITEM)->_next)
#define PREV(ITEM) ((ITEM)->_prev)
#define IN_QUEUE(ITEM) ((ITEM)->_in_queue)
#define DECLARE_DOUBLE_QUEUE_NODE(TYPE)   \
public:  											\
   TYPE *_next;   								\
   TYPE *_prev;   								\
   bool _in_queue

#define DEFINE_INTRUSIVE_DOUBLE_DATA() \
	typedef T* node_type; 					\
   volatile node_type head;				\
   volatile node_type tail

#ifdef INSTRUMENTATION
#define QUEUE_INCREMENT_TOTAL() ++total
#define QUEUE_DECREMENT_TOTAL() --total
#define QUEUE_DEFINE_TOTAL_SIZE() inline size_t size(void) const { return total; }
#define QUEUE_DEFINE_TOTAL() utils::atomic<size_t> total
#define DEFINE_INTRUSIVE_CONSTRUCTOR(TYPE) 	\
   explicit TYPE(void) : 							\
      head(NULL), tail(NULL), total(0)			\
   { }
#else
#define QUEUE_INCREMENT_TOTAL()
#define QUEUE_DECREMENT_TOTAL()
#define QUEUE_DEFINE_TOTAL_SIZE()
#define QUEUE_DEFINE_TOTAL()
#define DEFINE_INTRUSIVE_CONSTRUCTOR(TYPE)	\
	explicit TYPE(void): 							\
		head(NULL), tail(NULL)						\
	{ }
#endif

#define DEFINE_INTRUSIVE_IN_QUEUE()						\
	static inline bool in_queue(node_type node)		\
	{																\
   	return IN_QUEUE(node);								\
	}

#define DEFINE_INTRUSIVE_DOUBLE_EMPTY() inline bool empty(void) const { return head == NULL; }

#define DEFINE_INTRUSIVE_DOUBLE_POP_IF_NOT_EMPTY() \
	if(empty())													\
      return false;											\
      															\
   assert(!empty());											\
   																\
   if(NEXT(NEXT(head)) == NULL)							\
      return false;											\
   																\
   data = head;												\
   head = NEXT(head);										\
   PREV(head) = NULL;										\
   																\
   PREV(data) = NULL;										\
   NEXT(data) = NULL;										\
   IN_QUEUE(data) = false;									\
   																\
	QUEUE_DECREMENT_TOTAL();								\
																	\
   return true

#define DEFINE_INTRUSIVE_DOUBLE_POP()					\
	if(empty())													\
   	return false;											\
   																\
   assert(!empty());											\
   																\
   data = head;												\
   head = NEXT(head);										\
   if(head)														\
   	PREV(head) = NULL;									\
   else															\
      tail = NULL;											\
      															\
   IN_QUEUE(data) = false;									\
      															\
   assert((head && PREV(head) == NULL) || !head);	\
   assert((head && tail) || (!head && !tail));		\
   																\
	QUEUE_DECREMENT_TOTAL();								\
																	\
   return true

#define DEFINE_INTRUSIVE_DOUBLE_OPS() 					\
	inline void push_head_node(node_type new_node)	\
	{																\
		QUEUE_INCREMENT_TOTAL();							\
																	\
		IN_QUEUE(new_node) = true;							\
		NEXT(new_node) = head;								\
																	\
		if(tail == NULL)										\
			tail = new_node;									\
		if(head != NULL)										\
			PREV(head) = new_node;							\
																	\
		head = new_node;										\
																	\
		assert(NEXT(tail) == NULL);						\
		assert(PREV(head) == NULL);						\
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
      PREV(new_node) = tail;								\
																	\
      if(tail)													\
         NEXT(tail) = new_node;							\
      else														\
         tail = new_node;									\
																	\
      IN_QUEUE(new_node) = true;							\
      NEXT(new_node) = NULL;								\
      tail = new_node;										\
																	\
      assert(tail == new_node);							\
      assert(NEXT(tail) == NULL);						\
   }

#define DEFINE_INTRUSIVE_MOVE_UP()		{				\
	if(!in_queue(node))										\
		return;													\
																	\
   node_type prev(PREV(node));							\
      															\
   if(prev == NULL) {										\
      assert(node == head);								\
      return;													\
   }																\
   																\
   node_type next(NEXT(node));							\
      															\
   NEXT(prev) = next;										\
   if(next)														\
    	PREV(next) = prev;									\
         														\
   if(next == NULL)											\
    	tail = prev;											\
      															\
   assert(tail != NULL);									\
   																\
   PREV(node) = NULL;										\
	NEXT(node) = head;										\
   IN_QUEUE(node) = true;									\
	PREV(head) = node;										\
   head = node;												\
																	\
   if(tail == NULL)											\
		tail = node;											\
}

#define DEFINE_INTRUSIVE_REMOVE()		{				\
	node_type prev(PREV(node));							\
	node_type next(NEXT(node));							\
																	\
	if(head == node)											\
		head = next;											\
	if(tail == node)											\
		tail = prev;											\
																	\
	if(prev)														\
		NEXT(prev) = next;									\
	if(next)														\
		PREV(next) = prev;									\
																	\
	NEXT(node) = NULL;										\
	PREV(node) = NULL;										\
	IN_QUEUE(node) = false;									\
}

#define DEFINE_INTRUSIVE_CONST_ITERATOR()				\
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
				cur = NEXT(cur);								\
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
