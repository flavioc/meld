
#ifndef SCHED_QUEUE_UNSAFE_QUEUE_HPP
#define SCHED_QUEUE_UNSAFE_QUEUE_HPP

#include "queue/node.hpp"
#include "sched/queue/intrusive.hpp"

namespace sched
{
	
template <class T>
class intrusive_double_unsafe_queue
{
private:

	DEFINE_INTRUSIVE_DOUBLE_DATA();

	QUEUE_DEFINE_TOTAL();

	DEFINE_INTRUSIVE_DOUBLE_OPS();
   
public:
	
	DEFINE_INTRUSIVE_CONST_ITERATOR();
   
	QUEUE_DEFINE_TOTAL_SIZE(); // size()
	DEFINE_INTRUSIVE_DOUBLE_EMPTY(); // empty()
	DEFINE_INTRUSIVE_IN_QUEUE(); // static in_queue()
   
   inline bool pop_if_not_empty(node_type& data)
   {
		DEFINE_INTRUSIVE_DOUBLE_POP_IF_NOT_EMPTY();
   }
   
   inline bool pop(node_type& data)
   {
		DEFINE_INTRUSIVE_DOUBLE_POP();
   }
   
   inline void push(node_type data)
   {
      push_tail_node(data);
   }

	inline void push_head(node_type data)
	{	
		push_head_node(data);
	}
   
   inline void move_up(node_type node)
   {
		DEFINE_INTRUSIVE_MOVE_UP();
   }

	inline void remove(node_type node)
	{
		DEFINE_INTRUSIVE_REMOVE();
	}

	DEFINE_INTRUSIVE_CONSTRUCTOR(intrusive_double_unsafe_queue);
   
   ~intrusive_double_unsafe_queue(void)
   {
      assert(empty());
   }
};
   
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