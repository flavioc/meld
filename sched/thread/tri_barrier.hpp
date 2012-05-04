
#ifndef SCHED_THREAD_TRI_BARRIER_HPP
#define SCHED_THREAD_TRI_BARRIER_HPP

#include "utils/atomic.hpp"

namespace sched
{

class tri_barrier
{
private:
   
   utils::atomic<size_t> total;
   
   THREAD_LOCAL thread_round_state;
   
public:
   
   explicit tri_barrier(const size_t _total):
      total(_total)
   {
   }
};

}

#endif
