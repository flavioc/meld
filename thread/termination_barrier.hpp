
#ifndef THREAD_TERMINATION_BARRIER_HPP
#define THREAD_TERMINATION_BARRIER_HPP

#include <atomic>

#include "vm/state.hpp"
#include "utils/macros.hpp"

namespace sched
{
   
class termination_barrier
{
private:
   
   std::atomic<size_t> active_threads;
   
   std::atomic<bool> done;
   
public:
   
   inline void reset(void) { done = false; }
   inline void set_done(void) { done = true; }
   
   inline void is_active(void)
   {
      active_threads++;
   }
   
   inline void is_inactive(void)
   {
      assert(active_threads > 0);
      if(--active_threads == 0)
         done = true;
   }
   
   inline size_t num_active(void) const { return active_threads; }
   
   inline bool all_finished(void) const { return done; }
   
   // we use this for MPI, because in MPI the counter can reach zero and 
   // and become positive since we can get new work from remote threads
   inline bool zero_active_threads(void) const { return active_threads == 0; }
   
   explicit termination_barrier(const size_t _num_threads):
      active_threads(_num_threads), done(false)
   {
   }
   
   ~termination_barrier(void) {}
};

}

#endif
