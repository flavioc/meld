
#ifndef MEM_THREAD_HPP
#define MEM_THREAD_HPP

#include "conf.hpp"

#include <boost/thread/thread.hpp>

#include "mem/pool.hpp"

namespace mem
{
   
void init(const size_t);

pool *get_pool(void);
   
void create_pool(const size_t); // XXX: must be process_id

void cleanup(const size_t);
  
}

#endif
