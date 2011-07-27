
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
   
   DEFINE_PADDING;
   
   volatile bool done;
   
public:
   
   inline void reset(void)
   {
      done = false;
   }
   
   inline void is_active(void)
   {
      assert(active_threads < vm::state::NUM_THREADS);
      active_threads++;
   }
   
   inline void is_inactive(void)
   {
      assert(active_threads > 0);
      if(active_threads == 1) {
         active_threads--;
         done = true;
      } else {
         active_threads--;
      }
   }
   
   inline size_t num_active(void) const { return active_threads; }
   
   inline bool all_finished(void) const { return done; }
   
   explicit termination_barrier(const size_t num_threads):
      active_threads(num_threads)
   {
      done = false;
   }
   
   ~termination_barrier(void) {}
};

}

#endif
