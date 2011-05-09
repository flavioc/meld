
#ifndef SCHED_QUEUE_HPP
#define SCHED_QUEUE_HPP

#include <boost/thread/mutex.hpp>

#include "mem/allocator.hpp"

namespace sched
{

template <class T>
class queue_node
{
public:
   T data;
   queue_node *next;
};

template <class T>
class queue_lock_free
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
      node *new_node(mem::allocator<node>().allocate(1));
      
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
      
      mem::allocator<node>().deallocate(take, 1);
      
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
   
   explicit queue_lock_free(void): head(NULL), tail(NULL), total(0) {}
};

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
      node *new_node(mem::allocator<node>().allocate(1));
      
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
      
      mem::allocator<node>().deallocate(take, 1);
      
      return el;
   }
   
   // append a lock free queue on this queue
   inline void snap(queue_lock_free<T>& q)
   {
      boost::mutex::scoped_lock l(mtx);
      
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
   
   ~safe_queue(void) {}
};

}

#endif
