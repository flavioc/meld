
#ifndef QUEUE_MACROS_HPP
#define QUEUE_MACROS_HPP

#include "conf.hpp"

#define QUEUE_INCREMENT_TOTAL() ++total
#define QUEUE_INCREMENT_TOTAL_N(N) total += (N)
#define QUEUE_DECREMENT_TOTAL() --total
#define QUEUE_DEFINE_TOTAL_SIZE() inline size_t size(void) const { return total; }
#define QUEUE_DEFINE_TOTAL() utils::atomic<size_t> total
#define QUEUE_DEFINE_TOTAL_SERIAL() size_t total
#define QUEUE_ZERO_TOTAL() total = 0
#define QUEUE_ASSERT_TOTAL_ZERO() assert(total == 0)

#endif
