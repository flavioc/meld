
#include <atomic>
#include <unordered_set>
#include <iostream>

#include "mem/stat.hpp"

namespace mem
{
   
#ifdef MEMORY_STATISTICS

using namespace std;

static atomic<int64_t> memory_in_use{0};
static atomic<int64_t> total_memory{0};
static atomic<int64_t> num_mallocs{0};
static thread_local int64_t memory_in_use_thread{0};
static thread_local int64_t total_memory_thread{0};
static thread_local int64_t num_mallocs_thread{0};
#ifdef MEMORY_ASSERT
static unordered_set<void*> allocated_set;
#endif

void
register_allocation(void *p, const size_t cnt, const size_t size)
{
   memory_in_use_thread += cnt * size;
#ifdef MEMORY_ASSERT
   auto it(allocated_set.find(p));
   if(it != allocated_set.end()) {
      cerr << "Pointer " << p << " was allocated twice" << endl;
      abort();
   }
   allocated_set.insert(p);
#endif
}

void
register_deallocation(void *p, const size_t cnt, const size_t size)
{
   memory_in_use_thread -= cnt * size;
#ifdef MEMORY_ASSERT
   auto it(allocated_set.find(p));
   if(it == allocated_set.end()) {
      cerr << "Pointer " << p << " was freed twice" << endl;
      abort();
   }
   allocated_set.erase(it);
#endif
}

void
register_malloc(const size_t size)
{
   num_mallocs_thread++;
   total_memory_thread += size;
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

size_t
get_memory_in_use(void)
{
   return memory_in_use;
}

void
merge_memory_statistics()
{
   memory_in_use += memory_in_use_thread;
   total_memory += total_memory_thread;
   num_mallocs += num_mallocs_thread;
}

#endif
   
}
