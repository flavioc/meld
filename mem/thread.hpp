
#ifndef MEM_THREAD_HPP
#define MEM_THREAD_HPP

#include "conf.hpp"

#include <boost/thread/thread.hpp>

#include "mem/pool.hpp"

namespace mem
{
   
pool *get_pool(void);
   
void create_pool(void);
void ensure_pool(void);
void delete_pool(void);

void cleanup(const size_t);
  
}

#endif
