
#ifndef QUEUE_SAFE_DOUBLE_QUEUE_HPP
#define QUEUE_SAFE_DOUBLE_QUEUE_HPP

#include "utils/mutex.hpp"
#include "queue/intrusive_implementation.hpp"

namespace queue
{
   
template <class T>
class intrusive_safe_double_queue
{
public:

	QUEUE_DEFINE_INTRUSIVE_DOUBLE_DATA();

   utils::mutex mtx;

	QUEUE_DEFINE_INTRUSIVE_DOUBLE_OPS();
   
	QUEUE_DEFINE_TOTAL_SIZE(); // size()
	QUEUE_DEFINE_INTRUSIVE_DOUBLE_EMPTY(); // empty()
   
   inline bool pop_if_not_empty(node_type& data, const queue_id_t new_state)
   {
      MUTEX_LOCK_GUARD(mtx, normal_lock);
      
		QUEUE_DEFINE_INTRUSIVE_DOUBLE_POP_IF_NOT_EMPTY();
   }

   inline bool do_pop_head(node_type& data, const queue_id_t new_state)
   {
		QUEUE_DEFINE_INTRUSIVE_DOUBLE_POP();
   }

   inline bool pop_head(node_type& data, const queue_id_t new_state)
   {
      MUTEX_LOCK_GUARD(mtx, normal_lock);
      
      return do_pop_head(data, new_state);
   }

   inline bool pop_tail(node_type& data, const queue_id_t new_state)
   {
      MUTEX_LOCK_GUARD(mtx, normal_lock);

      QUEUE_DEFINE_INTRUSIVE_DOUBLE_POP_TAIL();
   }

   inline size_t pop_tail_half(T **buffer, const size_t max, const queue_id_t new_state)
   {
      MUTEX_LOCK_GUARD(mtx, normal_lock);

      size_t half = std::min(max, total / 2);

      if(half == 0)
         return 0;

      for(size_t i(0); i < half; ++i) {
         buffer[i] = head;
         __INTRUSIVE_QUEUE(head) = new_state;
         LOG_NORMAL_OPERATION();
         head = (node_type)__INTRUSIVE_NEXT(head);
      }
      // head will not be nullptr here.
      assert(head != nullptr);
      __INTRUSIVE_PREV(head) = nullptr;
      total -= half;
      return half;
   }
   
   inline void push_tail(node_type data LOCKING_STAT_FLAG)
   {
      MUTEX_LOCK_GUARD_FLAG(mtx, normal_lock, coord_normal_lock);
   
      push_tail_node(data);
   }

	inline void push_head(node_type data)
	{
      MUTEX_LOCK_GUARD(mtx, normal_lock);
		
		push_head_node(data);
	}
   
	QUEUE_DEFINE_INTRUSIVE_IN_QUEUE(); // in_queue()
   
   inline void move_up(node_type node)
   {
      MUTEX_LOCK_GUARD(mtx, normal_lock);

      if(!in_queue(node))
         return;
    
		QUEUE_DEFINE_INTRUSIVE_MOVE_UP();
   }

   inline bool do_remove(node_type node, const queue_id_t new_state)
   {
      if(!in_queue(node))
         return false;
		
		QUEUE_DEFINE_INTRUSIVE_REMOVE(node);
      return true;
   }

	inline bool remove(node_type node, const queue_id_t new_state LOCKING_STAT_FLAG)
	{
      MUTEX_LOCK_GUARD_FLAG(mtx, normal_lock, coord_normal_lock);

      return do_remove(node, new_state);
	}
	
	QUEUE_DEFINE_INTRUSIVE_CONSTRUCTOR(intrusive_safe_double_queue);

   explicit intrusive_safe_double_queue() {}
   
   ~intrusive_safe_double_queue(void)
   {
   }
};
   
}

#endif
