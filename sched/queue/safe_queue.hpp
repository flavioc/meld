
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
      
      assert(tail != NULL && head != NULL);
      
      tail->next = new_node;
      tail = new_node;
      
      assert(head != tail);
      assert(tail == new_node);
      assert(tail->next == NULL);
   }
   
public:
   
   inline const bool empty(void) const { return head == tail; }
   
   inline void push(T el)
   {
      node *new_node(new node());
      
      new_node->data = el;
      new_node->next = NULL;
      
      push_node(new_node);
   }
   
   inline T pop(void)
   {
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
      
      boost::mutex::scoped_lock l(mtx);
         
      tail->next = q.head;
      tail = q.tail;
      
      assert(q.tail = (node*)tail);
   }
   
   explicit safe_queue(void) {
      // sentinel node XXX use non-thread pool
      head = tail = (node*)malloc(sizeof(node));
   }
   
   ~safe_queue(void)
   {
      assert(head == tail);
      delete head;
   }
};

}

#endif
