
#ifndef SCHED_QUEUE_UNSAFE_QUEUE_COUNT_HPP
#define SCHED_QUEUE_UNSAFE_QUEUE_COUNT_HPP

#include "sched/queue/node.hpp"

namespace sched
{
   
// no safety of operations for this queue
// also does counting of elements
template <class T>
class unsafe_queue_count
{
public:
   
   typedef queue_node<T> node;
   node *head;
   node *tail;
   size_t total;
   
   inline const bool empty(void) const
   {
      return head == NULL;
   }
   
   inline const size_t size(void) const
   {
      return total;
   }
   
   inline void push(T el)
   {
      node *new_node(new node());
      
      new_node->data = el;
      new_node->next = NULL;
      
      if(head == NULL) {
         assert(tail == NULL);
         head = tail = new_node;
      } else {
         tail->next = new_node;
         tail = new_node;
      }
      
      assert(tail == new_node);
      
      ++total;
   }
   
   inline T pop(void)
   {
      node *take(head);
      
      assert(head != NULL);
   
      if(head == tail)
         head = tail = NULL;
      else
         head = head->next;
      
      assert(head != take);
      assert(take->next == head);
      
      T el(take->data);
      
      delete take;
      
      --total;
      
      return el;
   }
   
   inline void clear(void)
   {
      head = tail = NULL;
      total = 0;
      
      assert(head == NULL);
      assert(tail == NULL);
      assert(total == 0);
   }
   
   explicit unsafe_queue_count(void): head(NULL), tail(NULL), total(0) {}
   
   ~unsafe_queue_count(void)
   {
      assert(head == NULL);
      assert(tail == NULL);
      assert(total == 0);
   }
};

}

#endif
