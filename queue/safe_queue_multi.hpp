
#ifndef QUEUE_SAFE_QUEUE_MULTI_HPP
#define QUEUE_SAFE_QUEUE_MULTI_HPP

#include "conf.hpp"
#include "utils/spinlock.hpp"
#include "utils/atomic.hpp"
#include "queue/node.hpp"
#include "queue/macros.hpp"

namespace queue
{
   
template <class T>
class safe_queue_multi
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
   
   explicit safe_queue_multi(void):
      tail(NULL)
   {
      head = new node();
      head->next = NULL;
      tail = head;
		QUEUE_ZERO_TOTAL();
   }
   
   ~safe_queue_multi(void)
   {
      assert(empty());
		QUEUE_ASSERT_TOTAL_ZERO();
      delete head;
   }
};
   
}

#endif
