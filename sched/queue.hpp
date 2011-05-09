
#ifndef SCHED_QUEUE_HPP
#define SCHED_QUEUE_HPP

#include <boost/thread/mutex.hpp>

#include "mem/base.hpp"

namespace sched
{

template <class T>
class queue_node: public mem::base< queue_node<T> >
{
public:
   T data;
   queue_node *next;
};

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
      
      if(head == NULL)
         head = tail = new_node;
      else {
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

// no safety of operations for this queue
template <class T>
class unsafe_queue
{
public:
   
   typedef queue_node<T> node;
   node *head;
   node *tail;
   
   inline const bool empty(void) const
   {
      return head == NULL;
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
      
      assert(tail == new_node);
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
      
      return el;
   }
   
   explicit unsafe_queue(void): head(NULL), tail(NULL) {}
   
   ~unsafe_queue(void)
   {
      assert(head == NULL);
      assert(tail == NULL);
   }
};

// this queue ensures safety for multiple threads
// doing push and one thread doing pop
template <class T>
class safe_queue
{
private:
   
   typedef queue_node<T> node;
   
   mutable node *head;
   mutable node *tail;
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
      node *take(head);
      
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
      
      assert(q.tail = tail);
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
