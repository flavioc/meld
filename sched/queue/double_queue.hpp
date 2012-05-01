
#ifndef SCHED_QUEUE_DOUBLE_QUEUE_HPP
#define SCHED_QUEUE_DOUBLE_QUEUE_HPP

#include "conf.hpp"
#include "utils/spinlock.hpp"
#include "utils/atomic.hpp"

namespace sched
{
   
template <class T>
class double_queue
{
private:

#define NEXT(ITEM) ((ITEM)->_next)
#define PREV(ITEM) ((ITEM)->_prev)
#define IN_QUEUE(ITEM) ((ITEM)->_in_queue)
#define DECLARE_DOUBLE_QUEUE_NODE(TYPE)   \
public:  \
   TYPE *_next;   \
   TYPE *_prev;   \
   bool _in_queue

   typedef T* node_type;
   volatile node_type head;
   volatile node_type tail;
	utils::spinlock mtx;
	
#ifdef INSTRUMENTATION
   utils::atomic<size_t> total;
#endif

   inline void push_node(node_type new_node)
   {
#ifdef INSTRUMENTATION
      total++;
#endif

      if(!head)
         head = new_node;

      PREV(new_node) = tail;
         
      if(tail)
         NEXT(tail) = new_node;
      else
         tail = new_node;
      
      IN_QUEUE(new_node) = true;
      NEXT(new_node) = NULL;
      tail = new_node;
      
      assert(tail == new_node);
      assert(NEXT(tail) == NULL);
   }
   
public:
   
#ifdef INSTRUMENTATION
   inline size_t size(void) const { return total; }
#endif
   
   inline bool empty(void) const { return head == NULL; }
   
   inline bool pop_if_not_empty(node_type& data)
   {
      if(empty())
         return false;
      
      utils::spinlock::scoped_lock l(mtx);
      
      if(empty())
         return false;
         
      assert(!empty());
      
      if(NEXT(NEXT(head)) == NULL)
         return false; // we must leave this queue with at least one element
      
      data = head;
      head = NEXT(head);
      PREV(head) = NULL;
      
      // clean up data
      PREV(data) = NULL;
      NEXT(data) = NULL;
      IN_QUEUE(data) = false;
         
#ifdef INSTRUMENTATION
      total--;
#endif
      return true;
   }
   
   inline bool pop(node_type& data)
   {
      if(empty())
         return false;
      
      {
         utils::spinlock::scoped_lock l(mtx);
      
         if(empty())
            return false;
      
         assert(!empty());
      
         data = head;
         head = NEXT(head);
         if(head)
            PREV(head) = NULL;
         else
            tail = NULL;
         
         IN_QUEUE(data) = false;
         
         assert((head && PREV(head) == NULL) || !head);
         assert((head && tail) || (!head && !tail));
      }
      
#ifdef INSTRUMENTATION
      total--;
#endif
      return true;
   }
   
   inline void push(node_type data)
   {
      utils::spinlock::scoped_lock l(mtx);
   
      push_node(data);
   }
   
   explicit double_queue(void):
      head(NULL), tail(NULL)
#ifdef INSTRUMENTATION
      , total(0)
#endif
   {
   }
   
   static inline bool in_queue(node_type node)
   {
      return IN_QUEUE(node);
   }
   
   inline void move_up(node_type node)
   {
      utils::spinlock::scoped_lock l(mtx);
      
      if(in_queue(node)) {
         node_type prev(PREV(node));
         
         if(prev == NULL) {
            assert(node == head);
            return;
         }
      
         node_type next(NEXT(node));
            
         if(prev)
            NEXT(prev) = next;
         if(next)
            PREV(next) = prev;
            
         if(next == NULL)
            tail = prev;
         
         assert(tail != NULL);
      }
      
      // now we put it in the head
      NEXT(node) = head;
      PREV(node) = NULL;
      IN_QUEUE(node) = true;
      head = node;
      if(tail == NULL)
         tail = node;
   }
   
   ~double_queue(void)
   {
      assert(empty());
#ifdef INSTRUMENTATION
      assert(total == 0);
#endif
   }
};

#undef NEXT
#undef PREV
#undef IN_QUEUE
   
}

#endif
