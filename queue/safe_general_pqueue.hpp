
#ifndef QUEUE_SAFE_GENERAL_PQUEUE_HPP
#define QUEUE_SAFE_GENERAL_PQUEUE_HPP

#include <queue>
#include <boost/thread/mutex.hpp>

namespace queue
{

template <class T, class P>
class general_pqueue
{
private:

   mutable boost::mutex delay_mtx;

   struct queue_item {
      T item;
      P priority;

      queue_item(const T _item, const P _prio): item(_item), priority(_prio) {}
   };

   struct queue_item_comparator {
      bool operator() (const queue_item& a, const queue_item& b) {
         return a.priority < b.priority;
      }
   };

   std::priority_queue<queue_item, std::vector<queue_item>, queue_item_comparator> data;

public:

   inline void push(T item, P priority)
   {
      queue_item qitem(item, priority);

      delay_mtx.lock();
      data.push(qitem);
      delay_mtx.unlock();
   }

   inline size_t size(void) const
   {
      return data.size();
   }

   inline bool empty(void) const
   {
      return data.empty();
   }

   inline P top_priority(void) const
   {
      delay_mtx.lock();
      P ret(data.top().priority);
      delay_mtx.unlock();

      return ret;
   }

   inline T pop(void)
   {
      delay_mtx.lock();
      T ret(data.top().item);
      data.pop();
      delay_mtx.unlock();
      return ret;
   }
};

}

#endif
