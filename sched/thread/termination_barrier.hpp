
#ifndef SCHED_THREAD_TERMINATION_BARRIER_HPP
#define SCHED_THREAD_TERMINATION_BARRIER_HPP

#include "vm/state.hpp"
#include "utils/atomic.hpp"
#include "utils/macros.hpp"

namespace sched
{
   
class termination_barrier
{
private:
   
   utils::atomic<size_t> active_threads;
   
public:
   
   inline void reset(void) {}
   
   inline void is_active(void)
   {
      assert(active_threads < vm::state::NUM_THREADS);
      active_threads++;
   }
   
   inline void is_inactive(void)
   {
      assert(active_threads > 0);
      active_threads--;
   }
   
   inline size_t num_active(void) const { return active_threads; }
   
   inline bool all_finished(void) const { return active_threads == 0; }
   
   explicit termination_barrier(const size_t num_threads):
      active_threads(num_threads)
   {
   }
   
   ~termination_barrier(void) {}
};

}

#endif
