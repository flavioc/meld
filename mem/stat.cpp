
#include "utils/atomic.hpp"
#include "mem/stat.hpp"

using namespace utils;

namespace mem
{
   
#ifdef MEMORY_STATISTICS

static atomic<size_t> memory_in_use(0);

void
register_allocation(const size_t cnt, const size_t size)
{
   memory_in_use += cnt * size;
}

void
register_deallocation(const size_t cnt, const size_t size)
{
   memory_in_use -= cnt * size;
}

const size_t
get_memory_in_use(void)
{
   return memory_in_use;
}

#endif
   
}