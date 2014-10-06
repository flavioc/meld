
#ifndef QUEUE_SAFE_GENERAL_PQUEUE_HPP
#define QUEUE_SAFE_GENERAL_PQUEUE_HPP

#include <queue>

#include "utils/mutex.hpp"

namespace queue
{

template <class T, class P>
class general_pqueue
{
private:

   mutable utils::mutex delay_mtx;

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
      std::lock_guard<utils::mutex l(delay_mtx);

      data.push(qitem);
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
      std::lock_guard<utils::mutex l(delay_mtx);
      return data.top().priority;
   }

   inline T pop(void)
   {
      std::lock_guard<utils::mutex l(delay_mtx);
      return data.top().item;
   }
};

}

#endif
