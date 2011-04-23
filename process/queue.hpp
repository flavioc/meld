
#ifndef PROCESS_QUEUE_HPP
#define PROCESS_QUEUE_HPP

#include <boost/thread/mutex.hpp>

#include "mem/allocator.hpp"

namespace process
{

template <class T>
class queue_node
{
public:
   T data;
   queue_node *next;
};

template <class T>
class wqueue_free
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
   
   inline void clear(void)
   {
      head = tail = NULL;
      total = 0;
      
      assert(head == NULL);
      assert(tail == NULL);
      assert(total == 0);
   }
   
   explicit wqueue_free(void): head(NULL), tail(NULL), total(0) {}
};

template <class T>
class wqueue
{
private:
   
   typedef queue_node<T> node;
   
   node *head;
   node *tail;
   boost::mutex mtx;
   
public:
   
   inline const bool empty(void) const { return head == NULL; }
   
   inline void push(T el)
   {
      node *new_node(mem::allocator<node>().allocate(1));
      
      new_node->data = el;
      new_node->next = NULL;
      
      mtx.lock();
      
      if(head == NULL)
         head = tail = new_node;
      else {
         assert(tail != NULL);
         tail->next = new_node;
         tail = new_node;
      }
      
      mtx.unlock();
   }
   
   inline T pop(void)
   {
      node *take(head);
      
      assert(head != NULL);
   
      if(head == tail) {
         mtx.lock();
         if(head == tail) {
            head = tail = NULL;
         }
         mtx.unlock();
      } else {
         head = head->next;
      }
      
      T el(take->data);
      
      mem::allocator<node>().deallocate(take, 1);
      
      return el;
   }
   
   inline void snap(wqueue_free<T>& q)
   {
      mtx.lock();
      
      if(head == NULL) {
         head = q.head;
         tail = q.tail;
      } else {
         assert(tail != NULL);
         
         tail->next = q.head;
         tail = q.tail;
      }
      
      assert(q.tail = tail);
      
      mtx.unlock();
   }
   
   explicit wqueue(void): head(NULL), tail(NULL) {}
   
   ~wqueue(void) {}
};

}

#endif
