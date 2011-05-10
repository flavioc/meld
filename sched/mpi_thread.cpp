
#include "sched/mpi_thread.hpp"

using namespace std;
using namespace vm;
using namespace process;
using namespace db;

namespace sched
{
   
volatile static bool iteration_finished;
   
vector<sched::base*>&
mpi_thread::start(const size_t num_threads)
{
   init_barriers(num_threads);
   
   iteration_finished = false;
   
   for(process_id i(0); i < num_threads; ++i)
      add_thread(new mpi_thread(i));
   
   return ALL_THREADS;
}

}