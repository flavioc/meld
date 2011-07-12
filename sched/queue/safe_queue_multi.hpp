
#ifndef SCHED_QUEUE_SAFE_QUEUE_MULTI_HPP
#define SCHED_QUEUE_SAFE_QUEUE_MULTI_HPP

#include <boost/thread/mutex.hpp>
#include <queue>

#include "conf.hpp"
#include "utils/spinlock.hpp"
#include "utils/atomic.hpp"

namespace sched
{
   
template <class T>
class safe_queue_multi
{
private:
   
   std::queue<T> cont;
	utils::spinlock mtx;
#ifdef INSTRUMENTATION
   utils::atomic<size_t> total;
#endif
   
public:
   
#ifdef INSTRUMENTATION
   inline const size_t size(void) const { return total; }
#endif
   
   inline const bool empty(void) const { return cont.empty(); }
   
   inline bool pop(T& data)
   {
      if(cont.empty())
         return false;
         
		utils::spinlock::scoped_lock l(mtx);
      
      if(cont.empty())
         return false;
      
      data = cont.front();
      
      cont.pop();
      
#ifdef INSTRUMENTATION
      total--;
#endif
      
      return true;
   }
   
   inline void push(T data)
   {
#ifdef INSTRUMENTATION
      total++;
#endif
		utils::spinlock::scoped_lock l(mtx);
   
      cont.push(data);
   }
   
   explicit safe_queue_multi(void)
#ifdef INSTRUMENTATION
      : total(0)
#endif
   {
   }
   
   ~safe_queue_multi(void)
   {
      assert(empty());
   }
};
   
}

#endif
