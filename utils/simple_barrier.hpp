
#ifndef UTILS_SIMPLE_BARRIER_HPP
#define UTILS_SIMPLE_BARRIER_HPP

#include "utils/atomic.hpp"

namespace utils
{

class simple_barrier
{
private:
   
   utils::atomic<size_t> cnt;
   const size_t save;
   volatile bool done;
   
public:
   
   inline void reset(void)
   {
      cnt = save;
      done = false;
   }
   
   inline void wait(void)
   {
      if(cnt == 1) {
         done = true;
      } else {
         cnt--;
         while(!done);
      }
   }
   
   explicit barrier(const size_t x):
      save(x)
   {
      reset(); 
   }   
};

}

#endif
