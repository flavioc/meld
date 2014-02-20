
#ifndef VM_COUNTER_HPP
#define VM_COUNTER_HPP

#include <iostream>

#include "utils/types.hpp"

namespace vm
{

#define COUNTER_TYPE uint16_t
#define COUNTER_MAX ((COUNTER_TYPE)0xFFFF)

struct counter
{
   public:

      inline void decrement_count(const size_t i)
      {
         COUNTER_TYPE *p((COUNTER_TYPE*)this + i);

         if(*p > 0)
            *p = *p - 1;
      }

      inline void increment_count(const size_t i, const size_t begin, const size_t end)
      {
         COUNTER_TYPE *p((COUNTER_TYPE*)this + i);

         if(*p == COUNTER_MAX) {
            // do not add, remove others
            for(size_t j(begin); j <= end; ++j)
               if(j != i)
                  decrement_count(j);
         } else
            *p = *p + 1;
      }

      inline COUNTER_TYPE get_count(const size_t i)
      {
         return *((COUNTER_TYPE*)this + i);
      }

      inline void print(const size_t items)
      {
         COUNTER_TYPE *a((COUNTER_TYPE*)this);
         for(size_t i(0); i < items; ++i) {
            std::cout << (int)a[i] << std::endl;
         }
      }
};

inline counter*
create_counter(const size_t items)
{
   return (counter*)mem::allocator<COUNTER_TYPE>().allocate(items);
}

inline void
delete_counter(counter *c, const size_t items)
{
   mem::allocator<COUNTER_TYPE>().deallocate((COUNTER_TYPE*)c, items);
}
}

#endif
