#ifndef UTILS_CIRCULAR_BUFFER_HPP
#define UTILS_CIRCULAR_BUFFER_HPP

#include <iostream>

#include "mem/base.hpp"

// define a concurrent circular buffer
// that allows multiple producers and a single consumer

namespace utils
{

template <class T, int MAX_SIZE>
class circular_buffer: public mem::base
{
   private:

      const uint32_t total_size;
      T buffer[MAX_SIZE];
      volatile uint64_t start_length;

      inline uint32_t start(const uint64_t sl) const
      {
         return 0xFFFFFFFF & sl;
      }

      inline uint64_t make_start_length(const uint32_t start,
            const uint32_t length) const
      {
         return (((uint64_t)length) << 32) | start;
      }

      inline uint32_t length(const uint64_t sl) const
      {
         return sl >> 32;
      }

   public:

      uint32_t size(void) const
      {
         return length(start_length);
      }

      bool add(T x)
      {
         // add new element to buffer
         // multiple threads may be adding items to the buffer
         
         for(size_t i(0); i < 5; ++i) {
            const uint64_t copy(start_length);
            uint32_t len(length(copy));
            if(len >= total_size)
               return false;
            const uint32_t s(start(copy));

            const uint32_t pos = (s + len) % total_size;
            const uint64_t new_slen(make_start_length(s, len + 1));
            // attempt to change variables
            if(__sync_bool_compare_and_swap(&start_length, copy, new_slen)) {
               buffer[pos] = x;
               return true;
            }
         }
         return false;
      }

      size_t pop(T *ptr)
      {
         const uint32_t s(start(start_length));
         const uint32_t l(size());

         // we copy all available items to ptr
         // and resets counters to 0

         const size_t right_size(std::min(total_size - s, l));
         memcpy(ptr, buffer + s, sizeof(T) * right_size);
         if(right_size < l)
            memcpy(ptr + right_size, buffer, sizeof(T) * l - right_size);

         start_length = 0;

         return l;
      }

      explicit circular_buffer(const uint32_t size): total_size(size), start_length(0) {}
};

}

#endif
