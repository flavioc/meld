
#ifndef SCHED_NODES_THREAD_INTRUSIVE_HPP
#define SCHED_NODES_THREAD_INTRUSIVE_HPP

#include "mem/base.hpp"
#include "sched/nodes/thread.hpp"
#include "queue/double_queue.hpp"
#include "db/tuple.hpp"
#include "utils/spinlock.hpp"
#include "sched/base.hpp"
#include "process/work.hpp"

namespace sched
{

class thread_intrusive_node: public thread_node
{
private:
   
   DECLARE_DOUBLE_QUEUE_NODE(thread_intrusive_node);
	bool has_priority;

public:
	
	inline bool with_priority(void) const { return has_priority; }
	inline void unmark_priority(void) { has_priority = false; }
	inline void mark_priority(void) { has_priority = true; }
	
   explicit thread_intrusive_node(const db::node::node_id _id, const db::node::node_id _trans):
      thread_node(_id, _trans), has_priority(false)
   {
	}
   
   virtual ~thread_intrusive_node(void) {}
};

}

#endif
