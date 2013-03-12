#include <iostream>

#include "vm/state.hpp"
#include "process/remote.hpp"

using namespace process;
using namespace std;
using namespace boost;
using namespace vm;
using namespace db;

namespace process
{

remote *remote::self = NULL;
size_t remote::world_size(0);

void
remote::cache_values(const size_t world_size, const size_t nodes_per_remote, const size_t nodes_total)
{
   last_one = ((size_t)(get_rank() + 1) == world_size);
   
   total_nodes = nodes_per_remote;
   
   if(am_last_one())
      total_nodes += nodes_total - (nodes_per_remote * world_size);
      
   nodes_base = get_rank() * nodes_per_remote;
   nodes_per_proc = total_nodes / num_threads;

#ifndef USE_SIM
   assert(total_nodes > 0);
#endif
}

}
