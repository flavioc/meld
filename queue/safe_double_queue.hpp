
#ifndef QUEUE_SAFE_DOUBLE_QUEUE_HPP
#define QUEUE_SAFE_DOUBLE_QUEUE_HPP

#include "conf.hpp"
#include "utils/mutex.hpp"
#include "queue/intrusive_implementation.hpp"

namespace queue
{
   
template <class T>
class intrusive_safe_double_queue
{
private:

	QUEUE_DEFINE_INTRUSIVE_DOUBLE_DATA();

   utils::mutex mtx;

	QUEUE_DEFINE_INTRUSIVE_DOUBLE_OPS();
   
public:
   
	QUEUE_DEFINE_TOTAL_SIZE(); // size()
	QUEUE_DEFINE_INTRUSIVE_DOUBLE_EMPTY(); // empty()
   
   inline bool pop_if_not_empty(node_type& data, const queue_id_t new_state = queue_no_queue)
   {
      if(empty())
         return false;
      
      std::lock_guard<utils::mutex> l(mtx);
      LOCK_STAT(ready_lock);
      
		QUEUE_DEFINE_INTRUSIVE_DOUBLE_POP_IF_NOT_EMPTY();
   }
   
   inline bool pop_head(node_type& data, const queue_id_t new_state = queue_no_queue)
   {
      if(empty())
         return false;
      
      std::lock_guard<utils::mutex> l(mtx);
      LOCK_STAT(ready_lock);
      
		QUEUE_DEFINE_INTRUSIVE_DOUBLE_POP();
   }

   inline bool pop_tail(node_type& data, const queue_id_t new_state = queue_no_queue)
   {
      if(empty())
         return false;

      std::lock_guard<utils::mutex> l(mtx);
      LOCK_STAT(ready_lock);

      QUEUE_DEFINE_INTRUSIVE_DOUBLE_POP_TAIL();
   }
   
   inline void push_tail(node_type data)
   {
      std::lock_guard<utils::mutex> l(mtx);
      LOCK_STAT(ready_lock);
   
      push_tail_node(data);
   }

	inline void push_head(node_type data)
	{
      std::lock_guard<utils::mutex> l(mtx);
      LOCK_STAT(ready_lock);
		
		push_head_node(data);
	}
   
	QUEUE_DEFINE_INTRUSIVE_IN_QUEUE(); // in_queue()
   
   inline void move_up(node_type node)
   {
      std::lock_guard<utils::mutex> l(mtx);
      LOCK_STAT(ready_lock);

      if(!in_queue(node))
         return;
    
		QUEUE_DEFINE_INTRUSIVE_MOVE_UP();
   }

	inline bool remove(node_type node, const queue_id_t new_state)
	{
      std::lock_guard<utils::mutex> l(mtx);
      LOCK_STAT(ready_lock);

      if(!in_queue(node))
         return false;
		
		QUEUE_DEFINE_INTRUSIVE_REMOVE(node);
      return true;
	}
	
	QUEUE_DEFINE_INTRUSIVE_CONSTRUCTOR(intrusive_safe_double_queue);
   
   ~intrusive_safe_double_queue(void)
   {
   }
};
   
}

#endif
