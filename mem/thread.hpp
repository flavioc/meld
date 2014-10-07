
#ifndef MEM_THREAD_HPP
#define MEM_THREAD_HPP

#include "mem/pool.hpp"

namespace mem
{

extern __thread pool *mem_pool;
   
inline void ensure_pool(void)
{
#ifdef POOL_ALLOCATOR
   if(mem_pool == NULL)
      mem_pool = new pool();
#endif
}
  
}

#endif
