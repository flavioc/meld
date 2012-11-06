
#ifndef QUEUE_SAFE_DOUBLE_QUEUE_HPP
#define QUEUE_SAFE_DOUBLE_QUEUE_HPP

#include "conf.hpp"
#include "utils/spinlock.hpp"
#include "utils/atomic.hpp"
#include "queue/intrusive_implementation.hpp"

namespace queue
{
   
template <class T>
class intrusive_safe_double_queue
{
private:

	QUEUE_DEFINE_INTRUSIVE_DOUBLE_DATA();

	utils::spinlock mtx;

	QUEUE_DEFINE_INTRUSIVE_DOUBLE_OPS();
   
public:
   
	QUEUE_DEFINE_TOTAL_SIZE(); // size()
	QUEUE_DEFINE_INTRUSIVE_DOUBLE_EMPTY(); // empty()
   
   inline bool pop_if_not_empty(node_type& data)
   {
      if(empty())
         return false;
      
      utils::spinlock::scoped_lock l(mtx);
      
		QUEUE_DEFINE_INTRUSIVE_DOUBLE_POP_IF_NOT_EMPTY();
   }
   
   inline bool pop(node_type& data)
   {
      if(empty())
         return false;
      
      utils::spinlock::scoped_lock l(mtx);
      
		QUEUE_DEFINE_INTRUSIVE_DOUBLE_POP();
   }
   
   inline void push_tail(node_type data)
   {
      utils::spinlock::scoped_lock l(mtx);
   
      push_tail_node(data);
   }

	inline void push_head(node_type data)
	{
		utils::spinlock::scoped_lock l(mtx);
		
		push_head_node(data);
	}
   
	QUEUE_DEFINE_INTRUSIVE_IN_QUEUE(); // in_queue()
   
   inline void move_up(node_type node)
   {
      utils::spinlock::scoped_lock l(mtx);
    
		QUEUE_DEFINE_INTRUSIVE_MOVE_UP();
   }

	inline void remove(node_type node)
	{
		utils::spinlock::scoped_lock l(mtx);
		
		QUEUE_DEFINE_INTRUSIVE_REMOVE(node);
	}
	
	QUEUE_DEFINE_INTRUSIVE_CONSTRUCTOR(intrusive_safe_double_queue);
   
   ~intrusive_safe_double_queue(void)
   {
      assert(empty());
		QUEUE_ASSERT_TOTAL_ZERO();
   }
};
   
}

#endif
