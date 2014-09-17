
#include <iostream>
#include <unordered_set>

#include "mem/thread.hpp"
#ifdef INSTRUMENTATION
#include "sched/base.hpp"
#endif

using namespace boost;
using namespace std;

namespace mem
{
   
__thread pool *mem_pool(NULL);

}
