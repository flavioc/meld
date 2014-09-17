
#ifndef MEM_THREAD_HPP
#define MEM_THREAD_HPP

#include "conf.hpp"

#include <boost/thread/thread.hpp>

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
