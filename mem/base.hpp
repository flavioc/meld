
#ifndef MEM_BASE_HPP
#define MEM_BASE_HPP

#include <iostream>

#include "mem/allocator.hpp"

namespace mem
{

template <class T>
class base
{
public:
   
   inline void* operator new(size_t size)
   {
      return allocator<T>().allocate(1);
   }
   
   static inline void operator delete(void *ptr)
   {
      allocator<T>().deallocate((T*)ptr, 1);
   }
};

}

#endif
