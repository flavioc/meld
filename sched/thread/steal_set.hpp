
#ifndef SCHED_THREAD_STEAL_SET_HPP
#define SCHED_THREAD_STEAL_SET_HPP

#include "mem/base.hpp"
#include "queue/safe_linear_queue.hpp"
#include "sched/base.hpp"

namespace sched
{
   
class steal_set: public mem::base
{
private:
   
	queue::push_safe_linear_queue<sched::base*> requests;
   
public:

   MEM_METHODS(steal_set)
   
   inline bool empty(void) const { return requests.empty(); }
   
   inline sched::base *pop(void)
   {
      assert(!empty());
      
      return requests.pop();
   }
   
   inline void push(sched::base *sc)
   {
      requests.push(sc);
   }
   
   inline void clear(void)
   {
      while(!empty())
         pop();
   }
   
   explicit steal_set(void)
   {
   }
   
   ~steal_set(void)
   {
      assert(requests.empty());
   }
};

}

#endif
