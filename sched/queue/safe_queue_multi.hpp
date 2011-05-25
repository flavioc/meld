
#ifndef SCHED_QUEUE_SAFE_QUEUE_MULTI_HPP
#define SCHED_QUEUE_SAFE_QUEUE_MULTI_HPP

#include <boost/thread/mutex.hpp>
#include <queue>
#include "utils/spinlock.hpp"

namespace sched
{
   
template <class T>
class safe_queue_multi
{
private:
   
   std::queue<T> cont;
	 utils::spinlock mtx;
   
public:
   
   inline const bool empty(void) const { return cont.empty(); }
   
   inline bool pop(T& data)
   {
      if(cont.empty())
         return false;
         
			utils::spinlock::scoped_lock l(mtx);
      
      if(cont.empty()) {
         return false;
      }
      
      data = cont.front();
      
      cont.pop();
      
      return true;
   }
   
   inline void push(T data)
   {
		 utils::spinlock::scoped_lock l(mtx);
   
      cont.push(data);
   }
   
   ~safe_queue_multi(void)
   {
      assert(empty());
   }
};
   
}

#endif
