
#ifndef MEM_CENTER_HPP
#define MEM_CENTER_HPP

#include "mem/stat.hpp"
#include "mem/thread.hpp"

namespace mem
{

class center
{
   private:

      static size_t bytes_used;

   public:

      static void* allocate(size_t cnt, size_t sz)
      {
         register_allocation(cnt, sz);
#ifdef TRACK_MEMORY
         bytes_used += cnt * sz;
#endif
         
#ifdef POOL_ALLOCATOR
         return mem_pool->allocate(cnt * sz);
#else
         return ::operator new(cnt * sz);
#endif
      }

      static void deallocate(void *p, size_t cnt, size_t sz)
      {
         register_deallocation(cnt, sz);
#ifdef TRACK_MEMORY
         bytes_used -= cnt * sz;
#endif

#ifdef POOL_ALLOCATOR
         mem_pool->deallocate(p, cnt * sz);
#else
         (void)cnt;
         (void)sz;
         ::operator delete(p);
#endif
      }
};

}

#endif
