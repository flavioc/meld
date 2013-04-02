
#ifndef QUEUE_SAFE_QUEUE_HPP
#define QUEUE_SAFE_QUEUE_HPP

#include <boost/static_assert.hpp>

#include "conf.hpp"
#include "utils/atomic.hpp"
#include "queue/node.hpp"
#include "queue/unsafe_linear_queue.hpp"
#include "queue/macros.hpp"
#include "queue/iterators.hpp"
#include "utils/spinlock.hpp"

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
class push_safe_linear_queue
{
private:
   
   typedef queue_node<T> node;
   typedef special_queue_node<T> special_node;
   
   BOOST_STATIC_ASSERT(sizeof(special_node) == sizeof(node));
   
   volatile node *head;
   utils::atomic_ref<special_node*> tail;
	
	QUEUE_DEFINE_TOTAL();
   
   inline void push_node(special_node *new_node)
   {
		QUEUE_INCREMENT_TOTAL();
		
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
   
   inline void splice_headtail(special_node* qhead, special_node* qtail)
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
   
	QUEUE_DEFINE_TOTAL_SIZE(); // size()
   
   inline bool empty(void) const
	{
		return head == reinterpret_cast<node*>(tail.get());
	}
	
	QUEUE_DEFINE_LINEAR_CONST_ITERATOR_CLASS();
	
	inline const_iterator begin(void) const
	{ return const_iterator((node*)head->next); }
	inline const_iterator end(void) const
	{ return const_iterator(NULL); }
   
   inline void push(T el)
   {
      node *new_node(new node());
      
      new_node->data = el;
      new_node->next = NULL;
      
      push_node((special_node*)new_node);
   }

	inline T top(void)
	{
		assert(tail != NULL);
		assert(head != NULL);
		assert(head->next != NULL);
		
		return head->data;
	}
   
   inline T pop(void)
   {
		QUEUE_DECREMENT_TOTAL();
		
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
      
      assert(tail != NULL);
      assert(head != NULL);
      
      splice_headtail((special_node*)q.head, (special_node*)q.tail);
   }
   
   inline void splice(unsafe_linear_queue<T>& q)
   {
      assert(!q.empty());

		QUEUE_INCREMENT_TOTAL_N(q.size());

      assert(tail != NULL);
      assert(head != NULL);
      
      splice_headtail((special_node*)q.head, (special_node*)q.tail);
   }
   
   explicit push_safe_linear_queue(void):
      tail(NULL), total(0)
   {
      // sentinel node
      head = new node();
      head->next = NULL;
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

	utils::spinlock mtx;
	
	QUEUE_DEFINE_TOTAL();

   inline void push_node(node *new_node)
   {
		QUEUE_INCREMENT_TOTAL();
		
      assert(tail != NULL);
      assert(head != NULL);

      tail->next = new_node;
      tail = new_node;
      assert(tail->next == NULL);
   }
   
public:
   
	QUEUE_DEFINE_TOTAL_SIZE(); // size()
   
   inline bool empty(void) const { return head == tail; }
   
   inline bool pop_if_not_empty(T& data)
   {
      assert(tail != NULL);
      assert(head != NULL);
      
      if(empty())
         return false;
         
      utils::spinlock::scoped_lock l(mtx);
    
      if(empty())
         return false;
         
      assert(head->next != NULL);
      assert(!empty());
      
      if(head->next->next == NULL)
         return false; // we must leave this queue with at least one element
      
      node *take((node*)head->next);
      node *old((node*)head);
      
      head = take;
      
      delete old;
      
      assert(head == take);
      assert(take != NULL);
      
      data = take->data;

		QUEUE_DECREMENT_TOTAL();
		
      return true;
   }
   
   inline bool pop(T& data)
   {
      assert(tail != NULL);
      assert(head != NULL);
      
      if(empty())
         return false;
      
      utils::spinlock::scoped_lock l(mtx);
      
      if(empty())
         return false;
      
      assert(head->next != NULL);
      assert(!empty());
      
      node *take((node*)head->next);
      node *old((node*)head);
      
      head = take;
      
      delete old;
      
      assert(head == take);
      assert(take != NULL);
      
      data = take->data;

		QUEUE_DECREMENT_TOTAL();
		
      return true;
   }
   
   inline void push(T data)
   {
      node *new_node(new node());
      
      new_node->data = data;
      new_node->next = NULL;
      
		utils::spinlock::scoped_lock l(mtx);
   
      push_node(new_node);
   }
   
   explicit safe_linear_queue(void):
      tail(NULL)
#ifdef INSTRUMENTATION
      , total(0)
#endif
   {
      head = new node();
      head->next = NULL;
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
