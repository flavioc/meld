
#ifndef QUEUE_SAFE_QUEUE_HPP
#define QUEUE_SAFE_QUEUE_HPP

#include <boost/static_assert.hpp>

#include "conf.hpp"
#include "utils/atomic.hpp"
#include "queue/node.hpp"
#include "queue/unsafe_queue.hpp"
#include "queue/unsafe_queue_count.hpp"

namespace queue
{
  
/* special node to use during lock-free operations */ 
template <class T>
class special_queue_node: public mem::base
{
public:
   MEM_METHODS(special_queue_node<T>)

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
#ifdef INSTRUMENTATION
   utils::atomic<size_t> total;
#endif
   
   inline void push_node(special_node *new_node)
   {
#ifdef INSTRUMENTATION
      total++;
#endif
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
   
   inline void snap_headtail(special_node* qhead, special_node* qtail)
   {
      while (true) {
         special_node *last(tail.get());
         special_node *next(last->next.get());
         
         if(last == tail.get()) {
            if(next == NULL) {
               if(last->next.compare_test_set(next, qhead)) {
                  tail.compare_and_set(last, qtail);
                  return;
               }
            } else {
               tail.compare_and_set(last, next);
            }
         }
      }
   }
   
public:
   
#ifdef INSTRUMENTATION
   inline size_t size(void) const { return total; }
#endif
   
   inline bool empty(void) const { return head == reinterpret_cast<node*>(tail.get()); }
   
   inline void push(T el)
   {
      node *new_node(new node());
      
      new_node->data = el;
      new_node->next = NULL;
      
      push_node((special_node*)new_node);
   }
   
   inline T pop(void)
   {
#ifdef INSTRUMENTATION
      total--;
#endif
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

#ifdef INSTRUMENTATION
      total += q.size();
#endif
      
      assert(tail != NULL);
      assert(head != NULL);
      
      snap_headtail((special_node*)q.head, (special_node*)q.tail);
   }
   
   inline void snap(unsafe_queue<T>& q)
   {
      assert(!q.empty());

#ifdef INSTRUMENTATION
      total += q.size();
#endif

      assert(tail != NULL);
      assert(head != NULL);
      
      snap_headtail((special_node*)q.head, (special_node*)q.tail);
   }
   
   explicit safe_queue(void):
      tail(NULL)
#ifdef INSTRUMENTATION
      , total(0)
#endif
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
