
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
   
   inline bool pop_safe(T& el)
   {
      mtx.lock();
      
      node *take(head);
      
      if(take == NULL)
         return false;
      
      if(head == tail) {
         if(head == tail) {
            head = tail = NULL;
         }
      } else {
         head = head->next;
      }
      
      mtx.unlock();
      
      el = take->data;
      
      mem::allocator<node>().deallocate(take, 1);
      
      return true;
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
      }
      
      assert(head != take);
      assert(take->next == head);
      
      T el(take->data);
      
      mem::allocator<node>().deallocate(take, 1);
      
      return el;
   }
   
   inline void snap(wqueue_free<T>& q)
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
   
   inline node* steal(const size_t many)
   {
      if(many == 0)
         return NULL;
         
      size_t remain(many);
      
      boost::mutex::scoped_lock l(mtx);
      
      node* ret(head);
      node *more(head);
      node *prev(NULL);
      
      while(more != NULL && remain > 0) {
         --remain;
         prev = more;
         more = more->next;
      }
      
      if(prev != NULL)
         prev->next = NULL;
      
      assert(more != prev || head == NULL);
      head = more;
      
      if(head == NULL)
         tail = NULL;
      
      return ret;
   }
   
   inline void append(node *more)
   {
      node *rem;
      
      while(more != NULL) {
         rem = more->next;
         push_node(more);
         more = rem;
      }
   }
   
   explicit wqueue(void): head(NULL), tail(NULL) {}
   
   ~wqueue(void) {}
};

}

#endif
