
#include "sched/threaded.hpp"

using namespace boost;
using namespace std;

namespace sched
{
   
vector<base*> threaded::ALL_THREADS;
barrier* threaded::thread_barrier(NULL);
termination_barrier* threaded::term_barrier(NULL);

void
threaded::init_barriers(const size_t num_threads)
{
   thread_barrier = new barrier(num_threads);
   term_barrier = new termination_barrier(num_threads);
   
   ALL_THREADS.resize(num_threads);
}
   
}