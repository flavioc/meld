
#ifndef QUEUE_INTRUSIVE_HPP
#define QUEUE_INTRUSIVE_HPP

typedef int32_t queue_id_t;
#define queue_no_queue -1

// we put fields for the double queue and priority together in order to save space.
#define DECLARE_DOUBLE_QUEUE_NODE(TYPE)   \
public:  											\
   union __next {                         \
      TYPE *next;                         \
      int64_t pos;                        \
      __next(void): next(nullptr) {}         \
   } __intrusive_next;                    \
   union __prev {                         \
      TYPE *prev;                         \
      double priority;                    \
      __prev(void): prev(nullptr) {}         \
   } __intrusive_prev;                    \
   queue_id_t __intrusive_queue = queue_no_queue; \
   utils::byte __intrusive_extra_id

#define __INTRUSIVE_QUEUE(ITEM) ((ITEM)->__intrusive_queue)
#define __INTRUSIVE_NEXT(ITEM) ((ITEM)->__intrusive_next.next)
#define __INTRUSIVE_PREV(ITEM) ((ITEM)->__intrusive_prev.prev)
#define __INTRUSIVE_PRIORITY(ITEM) ((ITEM)->__intrusive_prev.priority)
#define __INTRUSIVE_POS(ITEM) ((ITEM)->__intrusive_next.pos)
#define __INTRUSIVE_EXTRA_ID(ITEM) ((ITEM)->__intrusive_extra_id)

#endif
