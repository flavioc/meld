
#ifndef SCHED_STEAL_SET_HPP
#define SCHED_STEAL_SET_HPP

#include "mem/base.hpp"
#include "sched/queue.hpp"
#include "sched/base.hpp"

namespace sched
{
   
class steal_set: public mem::base<steal_set>
{
private:
   
   safe_queue<sched::base*> requests;
   
public:
   
   inline const bool empty(void) const { return requests.empty(); }
   
   inline sched::base *pop(void)
   {
      assert(!empty());
      
      return requests.pop();
   }
   
   inline void push(sched::base *sc)
   {
      requests.push(sc);
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
