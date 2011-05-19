
#include <boost/thread/tss.hpp>
#include <vector>
#include <iostream>
#include <tr1/unordered_set>

#include "mem/thread.hpp"

using namespace boost;
using namespace std;

boost::mutex allocator_mtx;
std::tr1::unordered_set<void*> mem_set;

namespace mem
{

static pool *main_pool(new pool());
static thread_specific_ptr<pool> pools(NULL);
static vector<pool*> vec;

void
init(const size_t num_threads)
{
   vec.resize(num_threads);
}

void
create_pool(const size_t id)
{
   pool *pl(new pool());   
   pools.reset(pl);
   vec[id] = pl;
}

pool*
get_pool(void)
{
   pool *pool(pools.get());
   
   if(pool == NULL)
      return main_pool;
      
   return pool;
}

void
cleanup(const size_t num_threads)
{
   for(size_t i(0); i < num_threads; ++i)
      delete vec[i];
}

}
