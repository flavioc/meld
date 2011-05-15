#include "conf.hpp"

#include <vector>
#include <boost/thread/mutex.hpp>

#include "sched/thread/assert.hpp"
#include "vm/state.hpp"

using namespace boost;
using namespace std;
using namespace vm;

namespace sched
{

#ifdef ASSERT_THREADS

static mutex mtx;
static vector<size_t> total;

void
assert_thread_iteration(const size_t iteration)
{
   mutex::scoped_lock lock(mtx);
   
   total.push_back(iteration);
   
   assert(iteration == total[0]);
   
   if(total.size() == state::NUM_THREADS)
      total.clear();
}

#else

void
assert_thread_iteration(const size_t)
{
}

#endif

}