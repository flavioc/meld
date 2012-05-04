
#ifndef MEM_CENTER_HPP
#define MEM_CENTER_HPP

#include "conf.hpp"
#include "mem/stat.hpp"
#include "mem/thread.hpp"

namespace mem
{

class center
{
public:

   static void* allocate(size_t cnt, size_t sz);
   static void deallocate(void *p, size_t cnt, size_t sz);
};

}

#endif
