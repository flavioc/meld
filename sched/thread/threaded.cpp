
#include "sched/thread/threaded.hpp"

using namespace boost;
using namespace std;
using namespace utils;

namespace sched
{
   
vector<base*> threaded::ALL_THREADS;
tree_barrier* threaded::thread_barrier(NULL);
termination_barrier* threaded::term_barrier(NULL);

void
threaded::init_barriers(const size_t num_threads)
{
   thread_barrier = new tree_barrier(num_threads);
   term_barrier = new termination_barrier(num_threads);
   
   ALL_THREADS.resize(num_threads);
}
   
}
