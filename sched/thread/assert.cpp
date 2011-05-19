#include "conf.hpp"

#include <vector>
#include <boost/thread/mutex.hpp>

#include "sched/thread/assert.hpp"
#include "vm/state.hpp"

using namespace boost;
using namespace std;
using namespace vm;
using namespace utils;

namespace sched
{

#ifdef ASSERT_THREADS

static mutex mtx;
static vector<size_t> total;
static atomic<int> work(0);
static bool assert_work_count(true);

void
assert_thread_iteration(const size_t iteration)
{
   mutex::scoped_lock lock(mtx);
   
   total.push_back(iteration);
   
   assert(iteration == total[0]);
   
   if(total.size() == state::NUM_THREADS)
      total.clear();
}

void
assert_thread_push_work(void)
{
   work++;
}

void
assert_thread_pop_work(void)
{
   work--;
}

void
assert_thread_disable_work_count(void)
{
   assert_work_count = false;
}

void
assert_thread_end_iteration(void)
{
   if(!assert_work_count)
      return;
      
   if(work > 0)
      printf("Missing %d tuples to process\n", (size_t)work);
   assert(work == 0);
}

#else

void
assert_thread_iteration(const size_t)
{
}

void
assert_thread_push_work(void)
{
}

void
assert_thread_pop_work(void)
{
}

void
assert_thread_end_iteration(void)
{
}

void
assert_thread_disable_work_count(void)
{
}

#endif

}