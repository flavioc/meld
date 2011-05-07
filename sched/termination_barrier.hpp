
#ifndef SCHED_TERMINATION_BARRIER_HPP
#define SCHED_TERMINATION_BARRIER_HPP

namespace sched
{
   
class termination_barrier
{
private:
   
   mutable size_t active_threads;
   
public:
   
   inline void is_active(void) { __sync_fetch_and_add(&active_threads, 1); }
   inline void is_inactive(void) { __sync_fetch_and_sub(&active_threads, 1); }
   
   inline const bool all_finished(void) const { return active_threads == 0; }
   
   explicit termination_barrier(const size_t num_threads):
      active_threads(num_threads)
   {}
   
   ~termination_barrier(void) {}
};

}

#endif
