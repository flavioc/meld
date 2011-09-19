
#ifndef SCHED_QUEUE_UNSAFE_QUEUE_HPP
#define SCHED_QUEUE_UNSAFE_QUEUE_HPP

#include "sched/queue/node.hpp"

namespace sched
{
   
// no safety of operations for this queue
template <class T>
class unsafe_queue
{
public:
   
   typedef unsafe_queue_node<T> node;
   node *head;
   node *tail;
#ifdef INSTRUMENTATION
   size_t size;
#endif
   
   inline bool empty(void) const
   {
      return head == NULL;
   }
   
   inline void clear(void)
   {
      head = tail = NULL;
#ifdef INSTRUMENTATION
      size = 0;
#endif
   }
   
   inline void push(T el)
   {
      node *new_node(new node());
      
      new_node->data = el;
      new_node->next = NULL;
      
      if(head == NULL)
         head = tail = new_node;
      else {
         tail->next = new_node;
         tail = new_node;
      }
      
#ifdef INSTRUMENTATION
      size++;
#endif
      
      assert(tail == new_node);
   }
   
   inline T pop(void)
   {
      node *take(head);
      
      assert(head != NULL);
   
      if(head == tail)
         head = tail = NULL;
      else
         head = (node*)head->next;
      
      assert(head != take);
      assert(take->next == head);
      
      T el(take->data);
      
      delete take;
      
#ifdef INSTRUMENTATION
      size--;
#endif
      
      return el;
   }
   
   explicit unsafe_queue(void):
      head(NULL), tail(NULL)
#ifdef INSTRUMENTATION
      , size(0)
#endif
   {}
   
   ~unsafe_queue(void)
   {
      assert(head == NULL);
      assert(tail == NULL);
#ifdef INSTRUMENTATION
      size = 0;
#endif
   }
};

}

#endif