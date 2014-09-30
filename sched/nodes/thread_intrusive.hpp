
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
	
   double priority_level;
   bool updated_priority;
	
public:

   inline bool priority_changed(void) const
   {
      return updated_priority;
   }
   inline void mark_priority(void)
   {
      updated_priority = true;
   }

   inline void unmark_priority(void)
   {
      updated_priority = false;
   }

	inline double get_priority_level(void) { return priority_level; }
	inline void set_priority_level(const double level) { priority_level = level; }
	inline bool has_priority_level(void) const { return priority_level != 0.0; }

   explicit thread_intrusive_node(const db::node::node_id _id, const db::node::node_id _trans):
		thread_node(_id, _trans),
      INIT_DOUBLE_QUEUE_NODE(), INIT_PRIORITY_NODE(),
      priority_level(0.0),
      updated_priority(false)
   {
	}
   
   virtual ~thread_intrusive_node(void) {}
};

}

#endif
