
#ifndef QUEUE_UNSAFE_LINEAR_QUEUE_HPP
#define QUEUE_UNSAFE_LINEAR_QUEUE_HPP

#include <list>

#include "queue/macros.hpp"
#include "queue/iterators.hpp"
#include "queue/node.hpp"

namespace queue
{

#define QUEUE_DEFINE_LINEAR_BASE_DATA()	\
	typedef unsafe_queue_node<T> node;		\
	node *head;										\
	node *tail
#define QUEUE_DEFINE_LINEAR_DATA() 		\
	QUEUE_DEFINE_LINEAR_BASE_DATA();		\
	QUEUE_DEFINE_TOTAL_SERIAL()
	
#define QUEUE_DEFINE_LINEAR_DESTRUCTOR()	\
	assert(tail == NULL);						\
	assert(head == NULL);						\
	assert(empty())

#define QUEUE_DEFINE_LINEAR_CONSTRUCTOR() \
	head(NULL), tail(NULL)
	
#define QUEUE_DEFINE_LINEAR_CLEAR()	\
	head = tail = NULL;					\
	assert(head == NULL);				\
	assert(tail == NULL);				\
	assert(empty())

#define QUEUE_DEFINE_LINEAR_EMPTY()				\
   inline bool empty(void) const					\
   {														\
      return head == NULL;							\
   }

#define QUEUE_DEFINE_LINEAR_PUSH_NODE(ELEM)		\
	node *new_node(new node());						\
   															\
   new_node->data = ELEM;								\
   new_node->next = NULL;								\
   															\
   if(head == NULL) {									\
      assert(tail == NULL);							\
      head = new_node;									\
      tail = new_node;									\
   } else {													\
      tail->next = new_node;							\
      tail = new_node;									\
   }															\
																\
	assert(head != NULL);								\
   assert(tail == new_node)

#define QUEUE_DEFINE_LINEAR_POP_NODE()				\
   node *take(head);										\
   															\
   assert(head != NULL);								\
   															\
   if(head == tail)										\
      head = tail = NULL;								\
   else														\
      head = (node*)head->next;						\
   															\
   assert(head != take);								\
   assert(take->next == head);						\
   															\
   T el(take->data);										\
   															\
   delete take;											\
																\
   return el

#define QUEUE_DEFINE_LINEAR_POP_LIST(VEC)			\
	while(head != NULL)	{								\
		node *next(head->next);							\
		(VEC).push_back(head->data);					\
		delete head;										\
		head = next;										\
	}															\
	tail = NULL

#define QUEUE_DEFINE_LINEAR_TOP_NODE()				\
	assert(head != NULL);								\
	return head->data
	
// no safety of operations for this queue
template <class T>
class unsafe_linear_queue
{
public:
   
	QUEUE_DEFINE_LINEAR_DATA();
	
	QUEUE_DEFINE_LINEAR_CONST_ITERATOR();
	
	QUEUE_DEFINE_LINEAR_EMPTY(); // empty()
   
	QUEUE_DEFINE_TOTAL_SIZE(); // total()
   
   inline void clear(void)
   {
		QUEUE_DEFINE_LINEAR_CLEAR();
		QUEUE_ZERO_TOTAL();
   }
   
   inline void push(T el)
   {
		QUEUE_DEFINE_LINEAR_PUSH_NODE(el);
		QUEUE_INCREMENT_TOTAL();
   }
   
   inline T pop(void)
   {
		QUEUE_DECREMENT_TOTAL();
		QUEUE_DEFINE_LINEAR_POP_NODE();
   }

	template <class Alloc>
	inline void pop_list(std::list<T, Alloc>& vec)
	{
		QUEUE_DEFINE_LINEAR_POP_LIST(vec);
		QUEUE_ZERO_TOTAL();
	}

	inline T top(void)
	{
		QUEUE_DEFINE_LINEAR_TOP_NODE();
	}
   
   explicit unsafe_linear_queue(void):
      QUEUE_DEFINE_LINEAR_CONSTRUCTOR()
   {
		QUEUE_ZERO_TOTAL();
	}
   
   virtual ~unsafe_linear_queue(void)
   {
		QUEUE_DEFINE_LINEAR_DESTRUCTOR();
		QUEUE_ASSERT_TOTAL_ZERO();
   }
};

// no safety of operations for this queue
// also does counting of elements
template <class T>
class unsafe_linear_queue_count
{
public:
   
	QUEUE_DEFINE_LINEAR_BASE_DATA();
   size_t total;

	QUEUE_DEFINE_LINEAR_CONST_ITERATOR();
   
	QUEUE_DEFINE_LINEAR_EMPTY();
   
   inline size_t size(void) const
   {
      return total;
   }
   
   inline void push(T el)
   {
		QUEUE_DEFINE_LINEAR_PUSH_NODE(el);
      ++total;
   }

	inline T top(void)
	{
		QUEUE_DEFINE_LINEAR_TOP_NODE();
	}
   
   inline T pop(void)
   {
		--total;
		QUEUE_DEFINE_LINEAR_POP_NODE();
   }

	template <class Alloc>
	inline void pop_list(std::list<T, Alloc>& vec)
	{
		QUEUE_DEFINE_LINEAR_POP_LIST(vec);
		total = 0;
	}
   
   inline void clear(void)
   {
		QUEUE_DEFINE_LINEAR_CLEAR();
      
		total = 0;
      assert(total == 0);
   }
   
   explicit unsafe_linear_queue_count(void):
		QUEUE_DEFINE_LINEAR_CONSTRUCTOR(), total(0)
	{
	}
   
   ~unsafe_linear_queue_count(void)
   {
		QUEUE_DEFINE_LINEAR_DESTRUCTOR();
      assert(total == 0);
   }
};

}

#endif
