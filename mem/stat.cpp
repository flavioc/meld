
#include <atomic>
#include <unordered_set>
#include <iostream>
#include <list>

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
static thread_local int64_t memory_in_use_thread{0};
static thread_local int64_t total_memory_thread{0};
static thread_local std::list<std::pair<int64_t, int64_t>> memory_averages;
static thread_local int64_t average_memory_thread{0};
static thread_local int64_t number_steps_average{0};
static thread_local int64_t num_mallocs_thread{0};
#ifdef MEMORY_ASSERT
static unordered_set<void *> allocated_set;
#endif

void update_memory_average() {
   if(average_memory_thread > std::numeric_limits<int64_t>::max() - memory_in_use_thread) {
      // result will overflow
      memory_averages.push_back(make_pair(number_steps_average, average_memory_thread));
      number_steps_average = 1;
      average_memory_thread = memory_in_use_thread;
   } else {
      number_steps_average++;
      average_memory_thread += memory_in_use_thread;
   }
}

void register_allocation(void *p, const size_t cnt, const size_t size) {
   memory_in_use_thread += cnt * size;
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
   // compute average
   double total_steps{(double)number_steps_average};
   for(auto p : memory_averages) {
      total_steps += (double)p.first;
   }
   double average{0};
   average += ((double)number_steps_average / total_steps) * ((double)average_memory_thread / (double)number_steps_average);
   for(auto p : memory_averages) {
      average += ((double)p.first / total_steps) * ((double)p.second / (double)p.first);
   }
   total_memory_average += (int64_t)average;
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
