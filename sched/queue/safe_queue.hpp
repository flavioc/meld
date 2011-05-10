
#ifndef SCHED_QUEUE_SAFE_QUEUE_HPP
#define SCHED_QUEUE_SAFE_QUEUE_HPP

#include <boost/thread/mutex.hpp>

#include "sched/queue/node.hpp"
#include "sched/queue/unsafe_queue_count.hpp"

namespace sched
{

// this queue ensures safety for multiple threads
// doing push and one thread doing pop
template <class T>
class safe_queue
{
private:
   
   typedef queue_node<T> node;
   
   volatile node *head;
   volatile node *tail;
   boost::mutex mtx;
   
   inline void push_node(node *new_node)
   {
      boost::mutex::scoped_lock l(mtx);
      
      if(head == NULL)
         head = tail = new_node;
      else {
         assert(tail != NULL);
         tail->next = new_node;
         tail = new_node;
      }
      
      assert(head != NULL);
      assert(tail == new_node);
   }
   
public:
   
   inline const bool empty(void) const { return head == NULL; }
   
   inline void push(T el)
   {
      node *new_node(new node());
      
      new_node->data = el;
      new_node->next = NULL;
      
      push_node(new_node);
   }
   
   inline T pop(void)
   {
      node *take((node*)head);
      
      assert(head != NULL);
   
      if(head == tail) {
         mtx.lock();
         if(head == tail) {
            head = tail = NULL;
            mtx.unlock();
         } else {
            mtx.unlock();
            head = head->next;
         }
      } else {
         head = head->next;
         assert(take->next == head);
      }
      
      assert(head != take);
      
      T el(take->data);
      
      delete take;
      
      return el;
   }
   
   // append an unsafe queue on this queue
   inline void snap(unsafe_queue_count<T>& q)
   {
      boost::mutex::scoped_lock l(mtx);
      
      assert(q.size() > 0);
      assert(!q.empty());
      
      if(head == NULL) {
         head = q.head;
         tail = q.tail;
      } else {
         assert(tail != NULL);
         
         tail->next = q.head;
         tail = q.tail;
      }
      
      assert(q.tail = (node*)tail);
   }
   
   explicit safe_queue(void): head(NULL), tail(NULL) {}
   
   ~safe_queue(void)
   {
      assert(head == NULL);
      assert(tail == NULL);
   }
};

}

#endif
