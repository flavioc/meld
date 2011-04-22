
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
private:
   typedef queue_node<T> node;
   
   node *head;
   node *tail;
   
public:
   
   inline const bool empty(void) const
   {
      return head == NULL;
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
   }
   
   explicit wqueue_free(void): head(NULL), tail(NULL) {}
};

template <class T>
class wqueue
{
private:
   
   typedef queue_node<T> node;
   
   node *head;
   char coiso[1000];
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
         tail->next = new_node;
         tail = new_node;
      }
      
      mtx.unlock();
   }
   
   inline T pop(void)
   {
      node *take(head);
   
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
   
   explicit wqueue(void): head(NULL), tail(NULL) {}
   
   ~wqueue(void) {}
};

}

#endif
