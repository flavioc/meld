
#include <atomic>

#include "sched/thread/threaded.hpp"

using namespace std;
using namespace utils;

namespace sched
{
   
tree_barrier* threaded::thread_barrier(NULL);
termination_barrier* threaded::term_barrier(NULL);
std::atomic<size_t> threaded::total_in_agg(0);
std::atomic<size_t> threaded::round_state(1);

void
threaded::init_barriers(const size_t num_threads)
{
   thread_barrier = new tree_barrier(num_threads);
   term_barrier = new termination_barrier(num_threads);
   total_in_agg = num_threads;
}
   
}
