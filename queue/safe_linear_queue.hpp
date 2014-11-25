
#ifndef QUEUE_SAFE_QUEUE_HPP
#define QUEUE_SAFE_QUEUE_HPP

#include <atomic>
#include <iostream>

#include "utils/mutex.hpp"
#include "queue/node.hpp"
#include "queue/unsafe_linear_queue.hpp"
#include "queue/macros.hpp"
#include "queue/iterators.hpp"

namespace queue
{
  
// special node to use during lock-free operations
template <class T>
class special_queue_node: public mem::base
{
public:

   T data;
   std::atomic<special_queue_node*> next;
};

// this queue ensures safety for multiple threads
// doing push and one thread doing pop
template <class T>
class push_safe_linear_queue
{
private:
   
   typedef queue_node<T> node;
   typedef special_queue_node<T> special_node;
   
   static_assert(sizeof(special_node) == sizeof(node), "Nodes must have identical size.");
   
   node *head;
   std::atomic<special_node*> tail;
	
	QUEUE_DEFINE_TOTAL();
   
   inline void push_node(special_node *new_node)
   {
		QUEUE_INCREMENT_TOTAL();
		
      assert(tail != nullptr);
      assert(head != nullptr);
      
      while (true) {
         special_node *last(tail.load());
         special_node *next(last->next.load());
         
         if(last == tail.load()) {
            if(next == nullptr) {
               if(last->next.compare_exchange_strong(next, new_node)) {
                  tail.compare_exchange_strong(last, new_node);
                  return;
               }
            } else
               tail.compare_exchange_strong(last, next);
         }
      }
   }
   
   inline void splice_headtail(special_node* qhead, special_node* qtail)
   {
      while (true) {
         special_node *last(tail.load());
         special_node *next(last->next.load());
         
         if(last == tail.load()) {
            if(next == nullptr) {
               if(last->next.compare_exchange_strong(next, qhead)) {
                  tail.compare_exchange_strong(last, qtail);
                  return;
               }
            } else
               tail.compare_exchange_strong(last, next);
         }
      }
   }
   
public:
   
	QUEUE_DEFINE_TOTAL_SIZE(); // size()
   
   inline bool empty(void) const
	{
		return head == reinterpret_cast<node*>(tail.load());
	}
	
	QUEUE_DEFINE_LINEAR_CONST_ITERATOR_CLASS();
	
	inline const_iterator begin(void) const
	{ return const_iterator((node*)head->next); }
	inline const_iterator end(void) const
	{ return const_iterator(nullptr); }
   
   inline void push(T el)
   {
      node *new_node(new node());
      
      new_node->data = el;
      new_node->next = nullptr;
      
      push_node((special_node*)new_node);
   }

	inline T top(void)
	{
		assert(tail != nullptr);
		assert(head != nullptr);
		assert(head->next != nullptr);
		
		return head->data;
	}
   
   inline T pop(void)
   {
		QUEUE_DECREMENT_TOTAL();
		
      assert(tail != nullptr);
      assert(head != nullptr);
      assert(head->next != nullptr);
      
      node *take((node*)head->next);
      node *old((node*)head);
      
      head = take;
      
      delete old;
      
      assert(head == take);
      assert(take != nullptr);
      
      return take->data;
   }

   // not thread safe!
	template <class Alloc>
	inline void pop_list(std::list<T, Alloc>& vec)
	{
      while(!empty()) {
         node *take((node*)head->next);
         node *old((node *)head);

         head = take;

         vec.push_back(take->data);

         delete old;
      }

		QUEUE_ZERO_TOTAL();
	}
   
   // append an unsafe queue on this queue
   inline void splice(unsafe_linear_queue_count<T>& q)
   {
      assert(q.size() > 0);
      assert(!q.empty());

		QUEUE_INCREMENT_TOTAL_N(q.size());
      
      assert(tail != nullptr);
      assert(head != nullptr);
      
      splice_headtail((special_node*)q.head, (special_node*)q.tail);
   }
   
   inline void splice(unsafe_linear_queue<T>& q)
   {
      assert(!q.empty());

		QUEUE_INCREMENT_TOTAL_N(q.size());

      assert(tail != nullptr);
      assert(head != nullptr);
      
      splice_headtail((special_node*)q.head, (special_node*)q.tail);
   }
   
   explicit push_safe_linear_queue(void):
      tail(nullptr), total(0)
   {
      // sentinel node
      head = new node();
      head->next = nullptr;
      tail = (special_node*)head;
   }
   
   virtual ~push_safe_linear_queue(void)
   {
      assert(empty());
      delete head;
		QUEUE_ASSERT_TOTAL_ZERO();
   }
};

// allows multiple pushers and poppers
template <class T>
class safe_linear_queue
{
private:
   
   typedef queue_node<T> node;
   
   volatile node *head;
   volatile node *tail;

   utils::mutex mtx;
	
	QUEUE_DEFINE_TOTAL();

   inline void push_node(node *new_node)
   {
		QUEUE_INCREMENT_TOTAL();
		
      assert(tail != nullptr);
      assert(head != nullptr);

      tail->next = new_node;
      tail = new_node;
      assert(tail->next == nullptr);
   }
   
public:
   
	QUEUE_DEFINE_TOTAL_SIZE(); // size()
   
   inline bool empty(void) const { return head == tail; }
   
   inline bool pop_if_not_empty(T& data)
   {
      assert(tail != nullptr);
      assert(head != nullptr);
      
      if(empty())
         return false;
         
      std::lock_guard<utils::mutex> l(mtx);
    
      if(empty())
         return false;
         
      assert(head->next != nullptr);
      assert(!empty());
      
      if(head->next->next == nullptr)
         return false; // we must leave this queue with at least one element
      
      node *take((node*)head->next);
      node *old((node*)head);
      
      head = take;
      
      delete old;
      
      assert(head == take);
      assert(take != nullptr);
      
      data = take->data;

		QUEUE_DECREMENT_TOTAL();
		
      return true;
   }
   
   inline bool pop(T& data)
   {
      assert(tail != nullptr);
      assert(head != nullptr);
      
      if(empty())
         return false;
      
      std::lock_guard<utils::mutex> l(mtx);
      
      if(empty())
         return false;
      
      assert(head->next != nullptr);
      assert(!empty());
      
      node *take((node*)head->next);
      node *old((node*)head);
      
      head = take;
      
      delete old;
      
      assert(head == take);
      assert(take != nullptr);
      
      data = take->data;

		QUEUE_DECREMENT_TOTAL();
		
      return true;
   }
   
   inline void push(T data)
   {
      node *new_node(new node());
      
      new_node->data = data;
      new_node->next = nullptr;
      
      std::lock_guard<utils::mutex> l(mtx);
   
      push_node(new_node);
   }
   
   explicit safe_linear_queue(void):
      tail(nullptr)
#ifdef INSTRUMENTATION
      , total(0)
#endif
   {
      head = new node();
      head->next = nullptr;
      tail = head;
   }
   
   ~safe_linear_queue(void)
   {
      assert(empty());
		QUEUE_ASSERT_TOTAL_ZERO();
      delete head;
   }
};

}

#endif
