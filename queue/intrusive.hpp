
#ifndef QUEUE_INTRUSIVE_HPP
#define QUEUE_INTRUSIVE_HPP

#include "conf.hpp"

#define DECLARE_DOUBLE_QUEUE_NODE(TYPE)   \
public:  											\
   TYPE *__intrusive_next;   					\
   TYPE *__intrusive_prev;   					\
   bool __intrusive_in_queue

#endif
