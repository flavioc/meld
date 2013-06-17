
#ifndef SCHED_NODES_THREAD_INTRUSIVE_HPP
#define SCHED_NODES_THREAD_INTRUSIVE_HPP

#include "mem/base.hpp"
#include "sched/nodes/thread.hpp"
#include "queue/intrusive.hpp"
#include "db/tuple.hpp"
#include "utils/spinlock.hpp"
#include "sched/base.hpp"
#include "process/work.hpp"
#include "queue/safe_simple_pqueue.hpp"
#include "vm/state.hpp"

namespace sched
{

class thread_intrusive_node: public thread_node
{
	DECLARE_DOUBLE_QUEUE_NODE(thread_intrusive_node);
	DEFINE_PRIORITY_NODE(thread_intrusive_node);
	
private:
	
   heap_priority priority_level;
	
public:

   bool moving_around; // for debugging task stealing
	
	inline heap_priority get_priority_level(void) { return priority_level; }
	inline vm::int_val get_int_priority_level(void) { return priority_level.int_priority; }
	inline vm::float_val get_float_priority_level(void) { return priority_level.float_priority; }
	inline void set_priority_level(const heap_priority level) { priority_level = level; }
   inline void set_int_priority_level(const vm::int_val level) { priority_level.int_priority = level; }
   inline void set_float_priority_level(const vm::float_val level) { priority_level.float_priority = level; }
	inline bool has_priority_level(void) const {
      switch(all->PROGRAM->get_priority_type()) {
         case vm::FIELD_INT:
            return priority_level.int_priority > 0;
         case vm::FIELD_FLOAT:
            return priority_level.float_priority > 0.0;
         default:
            assert(false);
            return false;
      }
   }

	// xxx to remove
	bool has_been_prioritized;
   bool has_been_touched;

   explicit thread_intrusive_node(const db::node::node_id _id, const db::node::node_id _trans, vm::all *all):
		thread_node(_id, _trans, all),
      INIT_DOUBLE_QUEUE_NODE(), INIT_PRIORITY_NODE(),
      moving_around(false),
		has_been_prioritized(false),
      has_been_touched(false)
   {
      priority_level.float_priority = 0;
      priority_level.int_priority = 0;
	}
   
   virtual ~thread_intrusive_node(void) {}
};

}

#endif
