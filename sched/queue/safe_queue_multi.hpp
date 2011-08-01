
#ifndef SCHED_QUEUE_SAFE_QUEUE_MULTI_HPP
#define SCHED_QUEUE_SAFE_QUEUE_MULTI_HPP

#include "conf.hpp"
#include "utils/spinlock.hpp"
#include "utils/atomic.hpp"
#include "sched/queue/node.hpp"

namespace sched
{
   
template <class T>
class safe_queue_multi
{
private:
   
   typedef queue_node<T> node;
   
   volatile node *head;
   volatile node *tail;
	utils::spinlock mtx;
	
#ifdef INSTRUMENTATION
   utils::atomic<size_t> total;
#endif

   inline void push_node(node *new_node)
   {
#ifdef INSTRUMENTATION
      total++;
#endif
      assert(tail != NULL);
      assert(head != NULL);

      tail->next = new_node;
      tail = new_node;
      assert(tail->next == NULL);
   }
   
public:
   
#ifdef INSTRUMENTATION
   inline size_t size(void) const { return total; }
#endif
   
   inline bool empty(void) const { return head == tail; }
   
   inline bool pop(T& data)
   {
      assert(tail != NULL);
      assert(head != NULL);
      
      if(empty())
         return false;
      
      utils::spinlock::scoped_lock l(mtx);
      
      if(empty())
         return false;
      
      assert(head->next != NULL);
      assert(!empty());
      
      node *take((node*)head->next);
      node *old((node*)head);
      
      head = take;
      
      delete old;
      
      assert(head == take);
      assert(take != NULL);
      
      data = take->data;

#ifdef INSTRUMENTATION
      total--;
#endif
      return true;
   }
   
   inline void push(T data)
   {
      node *new_node(new node());
      
      new_node->data = data;
      new_node->next = NULL;
      
		utils::spinlock::scoped_lock l(mtx);
   
      push_node(new_node);
   }
   
   explicit safe_queue_multi(void):
      tail(NULL)
#ifdef INSTRUMENTATION
      , total(0)
#endif
   {
      head = new node();
      head->next = NULL;
      tail = head;
   }
   
   ~safe_queue_multi(void)
   {
      assert(empty());
#ifdef INSTRUMENTATION
      assert(total == 0);
#endif
      delete head;
   }
};
   
}

#endif
