
#ifndef QUEUE_INTRUSIVE_HPP
#define QUEUE_INTRUSIVE_HPP

typedef int32_t queue_id_t;
#define queue_no_queue -1

#define DECLARE_DOUBLE_QUEUE_NODE(TYPE)   \
public:  											\
   TYPE *__intrusive_next;   					\
   TYPE *__intrusive_prev;   					\
   queue_id_t __intrusive_queue

#define DEFINE_PRIORITY_NODE(TYPE)				\
public:													\
	double __intrusive_priority;			      \
	int __intrusive_pos;								\
	inline double get_intrusive_priority(void) const { return __intrusive_priority; }	\
	inline void set_intrusive_priority(const double n) { __intrusive_priority = n; }

#define INIT_PRIORITY_NODE()			\
	__intrusive_pos(0)
		
#define INIT_DOUBLE_QUEUE_NODE()		\
	__intrusive_next(NULL), __intrusive_prev(NULL), __intrusive_queue(queue_no_queue)

#define __INTRUSIVE_NEXT(ITEM) ((ITEM)->__intrusive_next)
#define __INTRUSIVE_PREV(ITEM) ((ITEM)->__intrusive_prev)
#define __INTRUSIVE_QUEUE(ITEM) ((ITEM)->__intrusive_queue)
#define __INTRUSIVE_PRIORITY(ITEM) ((ITEM)->__intrusive_priority)
#define __INTRUSIVE_POS(ITEM) ((ITEM)->__intrusive_pos)

#endif
