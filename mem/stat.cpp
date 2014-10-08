
#include <atomic>

#include "mem/stat.hpp"

namespace mem
{
   
#ifdef MEMORY_STATISTICS

using namespace std;

static atomic<size_t> memory_in_use(0);
static atomic<size_t> total_memory(0);
static atomic<size_t> num_mallocs(0);

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

size_t
get_memory_in_use(void)
{
   return memory_in_use;
}

void
register_malloc(const size_t size)
{
   num_mallocs++;
   total_memory += size;
}

size_t
get_num_mallocs(void)
{
   return num_mallocs;
}

size_t
get_total_memory(void)
{
   return total_memory;
}

#endif
   
}
