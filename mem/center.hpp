
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

      inline static void* allocate(size_t cnt, size_t sz) __attribute__((always_inline))
      {
#ifdef TRACK_MEMORY
         bytes_used += cnt * sz;
#endif
         
#ifdef POOL_ALLOCATOR
         void *p(mem_pool->allocate(cnt * sz));
#else
         void *p(::operator new(cnt * sz));
#endif
         register_allocation(p, cnt, sz);
         return p;
      }

      inline static void* allocate_cons(size_t sz) __attribute__((always_inline))
      {
#ifdef TRACK_MEMORY
         bytes_used += cnt * sz;
#endif
         
#ifdef POOL_ALLOCATOR
         void *p(mem_pool->allocate_cons(sz));
#else
         void *p(::operator new(sz));
#endif
         register_allocation(p, 1, sz);
         return p;
      }

      inline static void deallocate(void *p, size_t cnt, size_t sz) __attribute__((always_inline))
      {
         register_deallocation(p, cnt, sz);
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

      inline static void deallocate_cons(void *p, size_t sz) __attribute__((always_inline))
      {
         register_deallocation(p, 1, sz);
#ifdef TRACK_MEMORY
         bytes_used -= sz;
#endif

#ifdef POOL_ALLOCATOR
         mem_pool->deallocate_cons(p, sz);
#else
         (void)sz;
         ::operator delete(p);
#endif
      }
};

}

#endif
