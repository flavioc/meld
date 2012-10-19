
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

namespace sched
{

class thread_intrusive_node: public thread_node
{
	DECLARE_DOUBLE_QUEUE_NODE(thread_intrusive_node);
	DEFINE_PRIORITY_NODE(thread_intrusive_node);
	
private:
	
	bool has_priority;
	int priority_level;
	
public:
	
	typedef queue::heap_queue<process::node_work> prio_tuples_queue;
	prio_tuples_queue prioritized_tuples;
	
	heap_priority get_min_value(void) const { return prioritized_tuples.min_value(); }
	
	virtual bool has_prio_work(void) const { return !prioritized_tuples.empty(); }
	virtual bool has_normal_work(void) const { return thread_node::has_work(); }
	virtual bool has_work(void) const { return thread_node::has_work() || !prioritized_tuples.empty(); }

	inline bool with_priority(void) const { return has_priority; }
	inline void unmark_priority(void) { has_priority = false; }
	inline void mark_priority(void) { has_priority = true; }
	inline int get_priority_level(void) { return priority_level; }
	inline void set_priority_level(const int level) { priority_level = level; }

	// xxx to remove
	bool has_been_prioritized;
   bool has_been_touched;

   explicit thread_intrusive_node(const db::node::node_id _id, const db::node::node_id _trans):
		thread_node(_id, _trans),
      INIT_DOUBLE_QUEUE_NODE(), INIT_PRIORITY_NODE(),
		has_priority(false),
		priority_level(0),
		has_been_prioritized(false),
      has_been_touched(false)
   {
	}
   
   virtual ~thread_intrusive_node(void) {}
};

}

#endif
