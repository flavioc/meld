
#ifndef MEM_BASE_HPP
#define MEM_BASE_HPP

#include <iostream>

#include "mem/allocator.hpp"
#include "mem/center.hpp"

namespace mem
{

class base
{
public:
   
   static inline void* operator new(size_t sz)
   {
      return mem::center::allocate(sz, 1);
   }
   
   static inline void operator delete(void *ptr, size_t sz)
   {
      mem::center::deallocate(ptr, sz, 1);
   }
   
   virtual ~base(void) {}
};

#define MEM_METHODS(TYPE)

}

#endif
