
#ifndef QUEUE_INTRUSIVE_HPP
#define QUEUE_INTRUSIVE_HPP

#include "conf.hpp"

#define DECLARE_DOUBLE_QUEUE_NODE(TYPE)   \
public:  											\
   TYPE *__intrusive_next;   					\
   TYPE *__intrusive_prev;   					\
   bool __intrusive_in_queue

#define DEFINE_PRIORITY_NODE(TYPE)				\
public:													\
	bool __intrusive_in_priority_queue;			\
	heap_priority __intrusive_priority;			\
	int __intrusive_pos;								\
	inline heap_priority get_intrusive_priority(void) const { return __intrusive_priority; }	\
	inline void set_intrusive_priority(const heap_priority n) { __intrusive_priority = n; }

#define INIT_PRIORITY_NODE()			\
	__intrusive_in_priority_queue(false), __intrusive_pos(0)
		
#define INIT_DOUBLE_QUEUE_NODE()		\
	__intrusive_next(NULL), __intrusive_prev(NULL), __intrusive_in_queue(false)

#define __INTRUSIVE_NEXT(ITEM) ((ITEM)->__intrusive_next)
#define __INTRUSIVE_PREV(ITEM) ((ITEM)->__intrusive_prev)
#define __INTRUSIVE_IN_QUEUE(ITEM) ((ITEM)->__intrusive_in_queue)
#define __INTRUSIVE_IN_PRIORITY_QUEUE(ITEM) ((ITEM->__intrusive_in_priority_queue))
#define __INTRUSIVE_PRIORITY(ITEM) ((ITEM)->__intrusive_priority)
#define __INTRUSIVE_POS(ITEM) ((ITEM)->__intrusive_pos)

#endif
