
#ifndef QUEUE_SAFE_COMPLEX_PQUEUE_HPP
#define QUEUE_SAFE_COMPLEX_PQUEUE_HPP

#include "queue/intrusive_implementation.hpp"

namespace queue
{
	
template <class T>
class intrusive_safe_complex_pqueue
{
private:
	
	QUEUE_DEFINE_INTRUSIVE_DOUBLE_DATA();
	
	inline void push_node(node_type new_node, const int prio)
	{
		QUEUE_INCREMENT_TOTAL();
		
		__INTRUSIVE_IN_PRIORITY_QUEUE(new_node) = true;
		__INTRUSIVE_PRIORITY(new_node) = prio;
		
		if(empty()) {
			head = tail = new_node;
			__INTRUSIVE_PREV(head) = NULL;
			__INTRUSIVE_NEXT(head) = NULL;
			return;
		}
		
		if(__INTRUSIVE_PRIORITY(tail) < prio) {
			// push node at the end of queue
			__INTRUSIVE_NEXT(tail) = new_node;
			__INTRUSIVE_PREV(new_node) = tail;
			__INTRUSIVE_NEXT(new_node) = NULL;
			tail = new_node;
			return;
		}
		
		node_type cur(head);
		
		while (cur && __INTRUSIVE_PRIORITY(cur) < prio)
			cur = __INTRUSIVE_NEXT(cur);
		
		// cur must be non-NULL because we checked for tail priority before hand
		assert(cur != NULL);
		
		// node 'cur' will appear after the new node, since it has higher priority
		node_type prev(__INTRUSIVE_PREV(cur));
			
		if(prev) {
			__INTRUSIVE_NEXT(prev) = new_node;
			__INTRUSIVE_PREV(new_node) = prev;
		} else {
			head = new_node;
			__INTRUSIVE_PREV(new_node) = NULL;
		}
		
		__INTRUSIVE_PREV(cur) = new_node;
		__INTRUSIVE_NEXT(new_node) = cur;
	}
	
	node_type pop_node(void)
	{
		QUEUE_DECREMENT_TOTAL();
			
		node_type ret(head);
		
		head = __INTRUSIVE_NEXT(ret);
		
		if(head == NULL) {
			tail = NULL;
		} else {
			__INTRUSIVE_PREV(head) = NULL;
		}
		
		__INTRUSIVE_NEXT(ret) = NULL;
		assert(__INTRUSIVE_PREV(ret) == NULL);
		__INTRUSIVE_IN_PRIORITY_QUEUE(ret) = false;
		
		return ret;
	}
	
public:
	
	QUEUE_DEFINE_TOTAL_SIZE(); // size()
	QUEUE_DEFINE_INTRUSIVE_DOUBLE_EMPTY(); // empty()
	
	QUEUE_DEFINE_INTRUSIVE_CONSTRUCTOR(intrusive_safe_complex_pqueue);
	
	static inline bool in_queue(node_type node)
	{
   	return __INTRUSIVE_IN_PRIORITY_QUEUE(node);
	}
	
	void insert(node_type node, const int prio)
	{
		push_node(node, prio);
	}
	
	int min_value(void) const
	{
		return __INTRUSIVE_PRIORITY(head);
	}
	
	node_type pop(void)
	{
		if(empty())
			return NULL;
			
		return pop_node();
	}
	
	void move_node(node_type node, const int new_prio)
	{
		if(!__INTRUSIVE_IN_PRIORITY_QUEUE(node))
			return;
		
		const int old_prio(__INTRUSIVE_PRIORITY(node));
		// remove node from chain
		node_type next(__INTRUSIVE_NEXT(node));
		node_type prev(__INTRUSIVE_PREV(node));
		
		if(next) {
			__INTRUSIVE_PREV(next) = prev;
			__INTRUSIVE_NEXT(node) = NULL;
		}
		if(prev) {
			__INTRUSIVE_NEXT(prev) = next;
			__INTRUSIVE_PREV(node) = NULL;
		}
		
		if(head == node)
			head = next;
		if(tail == node)
			tail = prev;
		
		__INTRUSIVE_PRIORITY(node) = new_prio;
		
		if(old_prio < new_prio) {
			// put node at right
			//printf("old_prio < new_prio\n"); // DONE
			if(next == NULL) {
				if(tail == NULL) {
					//printf("no tail\n"); // DONE
					head = tail = node;
					assert(__INTRUSIVE_PREV(head) == NULL);
					assert(__INTRUSIVE_NEXT(head) == NULL);
					return;
				} else {
					//printf("with tail\n"); // DONE
					// prev is not NULL
					assert(prev != NULL);
					__INTRUSIVE_NEXT(prev) = node;
					__INTRUSIVE_PREV(node) = prev;
					tail = node;
					assert(__INTRUSIVE_NEXT(node) == NULL);
				}
			} else {
				//printf("with next\n"); // DONE
				node_type cur(next);
				
				while(cur && __INTRUSIVE_PRIORITY(cur) < new_prio)
					cur = __INTRUSIVE_NEXT(cur);
				
				if(cur == NULL) {
					//printf("with next tail\n"); // DONE
					__INTRUSIVE_NEXT(tail) = node;
					__INTRUSIVE_PREV(node) = tail;
					tail = node;
				} else {
					//printf("with next normal\n"); // DONE
					node_type prev(__INTRUSIVE_PREV(cur));
					
					__INTRUSIVE_NEXT(node) = cur;
					__INTRUSIVE_PREV(node) = prev;
					__INTRUSIVE_PREV(cur) = node;
					
					if(cur == head) {
						//printf("cur == head\n"); // DONE
						head = node;
						assert(prev == NULL);
					} else
						__INTRUSIVE_NEXT(prev) = node; // DONE
				}
			}
		} else {
			// old_prio >= new_prio
			// put node at left
			//printf("old_prio >= new_prio\n"); // DONE
			if(prev == NULL) {
				//printf("prev == NULL\n");
				if(head == NULL) {
					//printf("head == NULL\n"); // DONE
					head = tail = node;
					assert(__INTRUSIVE_PREV(head) == NULL);
					assert(__INTRUSIVE_NEXT(head) == NULL);
				} else {
					//printf("head != NULL\n"); // DONE
					__INTRUSIVE_PREV(head) = node;
					__INTRUSIVE_NEXT(node) = head;
					head = node;
					assert(__INTRUSIVE_PREV(head) == NULL);
				}
			} else {
				node_type cur(prev);
				
				while(cur && __INTRUSIVE_PRIORITY(cur) > new_prio)
					cur = __INTRUSIVE_PREV(cur);
				
				if(cur == NULL) {
					//printf("cur == NULL\n"); // DONE
					__INTRUSIVE_NEXT(node) = head;
					__INTRUSIVE_PREV(head) = node;
					head = node;
				} else {
					//printf("cur != NULL\n"); // DONE
					node_type next(__INTRUSIVE_NEXT(cur));
					
					__INTRUSIVE_PREV(node) = cur;
					__INTRUSIVE_NEXT(node) = next;
					__INTRUSIVE_NEXT(cur) = node;
					
					if(cur == tail) {
						//printf("cur == tail\n"); // DONE
						assert(__INTRUSIVE_PREV(node) == tail);
						tail = node;
						assert(__INTRUSIVE_NEXT(node) == NULL);
					} else {
						//printf("normal\n"); // DONE
						assert(next != NULL);
						__INTRUSIVE_PREV(next) = node;
					}
				}
			} 
		}
	}
	
	~intrusive_safe_complex_pqueue(void)
   {
      assert(empty());
		QUEUE_ASSERT_TOTAL_ZERO();
   }
};

}

#endif
