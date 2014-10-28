
#ifndef QUEUE_INTRUSIVE_HPP
#define QUEUE_INTRUSIVE_HPP

typedef int32_t queue_id_t;
#define queue_no_queue -1

static_assert(sizeof(double) == sizeof(intptr_t), "Double and intptr_t must have the same size.");

// we put fields for the double queue and priority together in order to save space.
#define DECLARE_DOUBLE_QUEUE_NODE(TYPE)   \
public:  											\
   union __next {                         \
      TYPE *next;                         \
      int64_t pos;                        \
      __next(void): next(NULL) {}         \
   } __intrusive_next;                    \
   union __prev {                         \
      TYPE *prev;                         \
      double priority;                    \
      __prev(void): prev(NULL) {}         \
   } __intrusive_prev;                    \
   queue_id_t __intrusive_queue = queue_no_queue

#define __INTRUSIVE_QUEUE(ITEM) ((ITEM)->__intrusive_queue)
#define __INTRUSIVE_NEXT(ITEM) ((ITEM)->__intrusive_next.next)
#define __INTRUSIVE_PREV(ITEM) ((ITEM)->__intrusive_prev.prev)
#define __INTRUSIVE_PRIORITY(ITEM) ((ITEM)->__intrusive_prev.priority)
#define __INTRUSIVE_POS(ITEM) ((ITEM)->__intrusive_next.pos)

#endif
