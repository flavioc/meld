
#ifndef SCHED_QUEUE_SAFE_QUEUE_HPP
#define SCHED_QUEUE_SAFE_QUEUE_HPP

#include <boost/static_assert.hpp>

#include "sched/queue/node.hpp"
#include "sched/queue/unsafe_queue_count.hpp"

namespace sched
{
  
/* special node to use during lock-free operations */ 
template <class T>
class special_queue_node
{
public:
   T data;
   utils::atomic_ref<special_queue_node*> next;
};

// this queue ensures safety for multiple threads
// doing push and one thread doing pop
template <class T>
class safe_queue
{
private:
   
   typedef queue_node<T> node;
   typedef special_queue_node<T> special_node;
   
   BOOST_STATIC_ASSERT(sizeof(special_node) == sizeof(node));
   
   volatile node *head;
   utils::atomic_ref<special_node*> tail;
   
   inline void push_node(special_node *new_node)
   {
      assert(tail != NULL);
      assert(head != NULL);
      
      while (true) {
         special_node *last(tail.get());
         special_node *next(last->next.get());
         
         if(last == tail.get()) {
            if(next == NULL) {
               if(last->next.compare_test_set(next, new_node)) {
                  tail.compare_and_set(last, new_node);
                  return;
               }
            } else {
               tail.compare_and_set(last, next);
            }
         }
      }
   }
   
public:
   
   inline const bool empty(void) const { return head == reinterpret_cast<node*>(tail.get()); }
   
   inline void push(T el)
   {
      node *new_node(new node());
      
      new_node->data = el;
      new_node->next = NULL;
      
      push_node((special_node*)new_node);
   }
   
   inline T pop(void)
   {
      assert(tail != NULL);
      assert(head != NULL);
      assert(head->next != NULL);
      
      node *take((node*)head->next);
      node *old((node*)head);
      
      head = take;
      
      delete old;
      
      assert(head == take);
      assert(take != NULL);
      
      return take->data;
   }
   
   // append an unsafe queue on this queue
   inline void snap(unsafe_queue_count<T>& q)
   {
      assert(q.size() > 0);
      assert(!q.empty());
      
      assert(tail != NULL);
      assert(head != NULL);
      
      special_node *new_tail = (special_node*)q.tail;
      special_node *new_next = (special_node*)q.head;
      
      while (true) {
         special_node *last(tail.get());
         special_node *next(last->next.get());
         
         if(last == tail.get()) {
            if(next == NULL) {
               if(last->next.compare_test_set(next, new_next)) {
                  tail.compare_and_set(last, new_tail);
                  return;
               }
            } else {
               tail.compare_and_set(last, next);
            }
         }
      }
   }
   
   explicit safe_queue(void):
      tail(NULL)
   {
      // sentinel node
      head = new node();
      head->next = NULL;
      tail = (special_node*)head;
   }
   
   ~safe_queue(void)
   {
      assert(empty());
      delete head;
   }
};

}

#endif
