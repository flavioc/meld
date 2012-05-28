
#ifndef QUEUE_SIMPLE_LINEAR_PQUEUE_HPP
#define QUEUE_SIMPLE_LINEAR_PQUEUE_HPP

#include "queue/unsafe_linear_queue.hpp"

// simple linear bounded priority queue
// not thread safe

namespace queue
{

template <class T>
class simple_linear_pqueue
{
private:
   
   typedef unsafe_linear_queue_count<T> queue;
   
   std::vector<queue> queues;
   size_t total;
   
public:
   
   inline bool empty(void) const { return total == 0; }
   inline size_t size(void) const { return total; }
   
   inline queue& get_queue(const size_t prio)
   {
      assert(prio < queues.size());
      return queues[prio];
   }
   
   void push(const T item, const size_t prio)
   {
      assert(prio < queues.size());
      
      queues[prio].push(item);
      ++total;
   }
   
   // no pop yet!
   
   void clear(void)
   {
      assert(total > 0);
      
      for(size_t i(0); i < queues.size(); ++i)
         get_queue(i).clear();
         
      total = 0;
   }
   
   explicit simple_linear_pqueue(const size_t range):
      total(0)
   {
      queues.resize(range);
   }
   
   ~simple_linear_pqueue(void)
   {
      assert(empty());
      assert(size() == 0);
   }
};

};

#endif
