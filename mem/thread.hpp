
#ifndef MEM_THREAD_HPP
#define MEM_THREAD_HPP

#include "mem/pool.hpp"

namespace mem
{

extern __thread pool *mem_pool;
   
inline void ensure_pool(void)
{
#ifdef POOL_ALLOCATOR
   if(mem_pool == nullptr)
      mem_pool = new pool();
#endif
}

inline void dump_pool()
{
#ifdef POOL_ALLOCATOR
   mem_pool->dump();
#endif
}
  
}

#endif
