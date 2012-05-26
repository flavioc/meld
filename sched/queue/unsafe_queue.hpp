
#ifndef SCHED_QUEUE_UNSAFE_QUEUE_HPP
#define SCHED_QUEUE_UNSAFE_QUEUE_HPP

#include "sched/queue/node.hpp"

namespace sched
{
   
// no safety of operations for this queue
template <class T>
class unsafe_queue
{
public:
   
   typedef unsafe_queue_node<T> node;

	class const_iterator
	{
		private:
			
			node *cur;
			
		public:
			
			inline T operator*(void)
			{
				assert(cur != NULL);
				return cur->data;
			}
			
			inline const_iterator& operator=(const const_iterator& it)
			{
				cur = it.cur;
				return *this;
			}
			
			inline void operator++(void)
			{
				assert(cur != NULL);
				cur = cur->next;
			}
			
			inline bool operator==(const const_iterator& it) const { return it.cur == cur; }
			
			explicit const_iterator(node *n): cur(n) {}
			
			explicit const_iterator(void): cur(NULL) {}
	};
	
   node *head;
   node *tail;
#ifdef INSTRUMENTATION
   size_t total;
#endif

	inline const_iterator begin(void) const { return const_iterator(head); }
	inline const_iterator end(void) const { return const_iterator(NULL); }
   
   inline bool empty(void) const
   {
      return head == NULL;
   }
   
#ifdef INSTRUMENTATION
   inline size_t size(void) const
   {
      return total;
   }
#endif
   
   inline void clear(void)
   {
      head = tail = NULL;
#ifdef INSTRUMENTATION
      total = 0;
#endif
   }
   
   inline void push(T el)
   {
      node *new_node(new node());
      
      new_node->data = el;
      new_node->next = NULL;
      
      if(head == NULL) {
         assert(tail == NULL);
         head = new_node;
         tail = new_node;
      } else {
         tail->next = new_node;
         tail = new_node;
      }
      
#ifdef INSTRUMENTATION
      total++;
#endif
      
      assert(head != NULL);
      assert(tail == new_node);
   }
   
   inline T pop(void)
   {
      node *take(head);
      
      assert(head != NULL);
      
      if(head == tail)
         head = tail = NULL;
      else
         head = (node*)head->next;
      
      assert(head != take);
      assert(take->next == head);
      
      T el(take->data);
      
      delete take;
      
#ifdef INSTRUMENTATION
      total--;
#endif

      return el;
   }
   
   explicit unsafe_queue(void):
      head(NULL), tail(NULL)
#ifdef INSTRUMENTATION
      , total(0)
#endif
   {}
   
   virtual ~unsafe_queue(void)
   {
      assert(head == NULL);
      assert(tail == NULL);
#ifdef INSTRUMENTATION
      assert(total == 0);
#endif
   }
};

}

#endif