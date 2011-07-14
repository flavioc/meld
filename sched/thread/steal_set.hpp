
#ifndef SCHED_THREAD_STEAL_SET_HPP
#define SCHED_THREAD_STEAL_SET_HPP

#include "mem/base.hpp"
#include "sched/queue/safe_queue.hpp"
#include "sched/base.hpp"

namespace sched
{
   
class steal_set: public mem::base<steal_set>
{
private:
   
   safe_queue<sched::base*> requests;
   
public:
   
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
