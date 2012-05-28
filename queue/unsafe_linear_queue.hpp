
#ifndef QUEUE_UNSAFE_LINEAR_QUEUE_HPP
#define QUEUE_UNSAFE_LINEAR_QUEUE_HPP

#include "queue/macros.hpp"
#include "queue/node.hpp"

namespace queue
{

// no safety of operations for this queue
template <class T>
class unsafe_linear_queue
{
public:
   
   typedef unsafe_queue_node<T> node;

   node *head;
   node *tail;
	QUEUE_DEFINE_TOTAL_SERIAL();

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
	
	inline const_iterator begin(void) const { return const_iterator(head); }
	inline const_iterator end(void) const { return const_iterator(NULL); }
   
   inline bool empty(void) const
   {
      return head == NULL;
   }
   
	QUEUE_DEFINE_TOTAL_SIZE(); // total()
   
   inline void clear(void)
   {
      head = tail = NULL;
		QUEUE_ZERO_TOTAL();
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
      
		QUEUE_INCREMENT_TOTAL();
      
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
      
		QUEUE_DECREMENT_TOTAL();

      return el;
   }
   
   explicit unsafe_linear_queue(void):
      head(NULL), tail(NULL)
   {
		QUEUE_ZERO_TOTAL();
	}
   
   virtual ~unsafe_linear_queue(void)
   {
      assert(head == NULL);
      assert(tail == NULL);
		QUEUE_ASSERT_TOTAL_ZERO();
   }
};

// no safety of operations for this queue
// also does counting of elements
template <class T>
class unsafe_linear_queue_count
{
public:
   
   typedef unsafe_queue_node<T> node;
   node *head;
   node *tail;
   size_t total;
   
   inline bool empty(void) const
   {
      return head == NULL;
   }
   
   inline size_t size(void) const
   {
      return total;
   }
   
   inline void push(T el)
   {
      node *new_node(new node());
      
      new_node->data = el;
      new_node->next = NULL;
      
      if(head == NULL) {
         assert(tail == NULL);
         head = tail = new_node;
      } else {
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
   
   explicit unsafe_linear_queue_count(void): head(NULL), tail(NULL), total(0) {}
   
   ~unsafe_linear_queue_count(void)
   {
      assert(head == NULL);
      assert(tail == NULL);
      assert(total == 0);
   }
};

}

#endif
