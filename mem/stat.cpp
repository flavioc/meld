
#include <atomic>
#include <unordered_set>
#include <iostream>

#include "mem/stat.hpp"
#include "vm/all.hpp"
#include "db/database.hpp"

namespace mem {

#ifdef MEMORY_STATISTICS

using namespace std;

static atomic<int64_t> memory_in_use{0};
static atomic<int64_t> total_memory{0};
static atomic<int64_t> num_mallocs{0};
static atomic<int64_t> total_memory_average{0};
static thread_local int64_t alloc_steps_thread{0};
static thread_local int64_t memory_in_use_thread{0};
static thread_local int64_t total_memory_thread{0};
static thread_local int64_t average_memory_thread{0};
static thread_local int64_t num_mallocs_thread{0};
#ifdef MEMORY_ASSERT
static unordered_set<void *> allocated_set;
#endif

void update_memory_average() {
   const double frac((double)((double)alloc_steps_thread - 1.0) /
                     (double)alloc_steps_thread);
   const double other_frac(1.0 - frac);
   average_memory_thread = (int64_t)(frac * (double)average_memory_thread +
                                     other_frac * (double)memory_in_use_thread);
}

void register_allocation(void *p, const size_t cnt, const size_t size) {
   memory_in_use_thread += cnt * size;
   alloc_steps_thread++;
   if (alloc_steps_thread == 1)
      average_memory_thread = memory_in_use_thread;
   else
      update_memory_average();

   (void)p;

#ifdef MEMORY_ASSERT
   auto it(allocated_set.find(p));
   if (it != allocated_set.end()) {
      cerr << "Pointer " << p << " was allocated twice" << endl;
      abort();
   }
   allocated_set.insert(p);
#endif
}

void register_deallocation(void *p, const size_t cnt, const size_t size) {
   memory_in_use_thread -= cnt * size;
   alloc_steps_thread++;
   update_memory_average();
   (void)p;

#ifdef MEMORY_ASSERT
   auto it(allocated_set.find(p));
   if (it == allocated_set.end()) {
      cerr << "Pointer " << p << " was freed twice" << endl;
      abort();
   }
   allocated_set.erase(it);
#endif
}

void register_malloc(const size_t size) {
   num_mallocs_thread++;
   total_memory_thread += size;
}

void merge_memory_statistics() {
   total_memory_average += average_memory_thread;
   memory_in_use += memory_in_use_thread;
   total_memory += total_memory_thread;
   num_mallocs += num_mallocs_thread;
}

void print_memory_statistics() {
   cout << "AvgMemoryInUse(KB) TotalMemoryAllocated(KB) TotalMemoryUse(KB) MallocsCalled TotalFacts"
        << endl;
   cout << total_memory_average / (vm::All->NUM_THREADS * 1024) << " "
        << total_memory / 1024 << " " << memory_in_use / 1024 << " "
        << num_mallocs << " " << vm::All->DATABASE->total_facts() << endl;
}

#endif
}
