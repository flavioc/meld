
#ifndef SCHED_NODES_THREAD_INTRUSIVE_HPP
#define SCHED_NODES_THREAD_INTRUSIVE_HPP

#include "mem/base.hpp"
#include "sched/nodes/thread.hpp"
#include "sched/queue/double_queue.hpp"
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

	typedef unsafe_queue<process::node_work> prio_queue;
	
	prio_queue pqueue;
	
	int prio_count;
	
public:
	
	inline bool has_prio_work(void) const { return prio_count > 0; }
	
	void add_priority_work(process::node_work& w)
	{
		pqueue.push(w);
		assert(prio_count >= 0);
		prio_count++;
	}
	
	process::node_work get_priority_work(void)
	{
		assert(!pqueue.empty());
		assert(prio_count >= 1);
		
		process::node_work ret(pqueue.pop());
		
		--prio_count;
		assert(prio_count >= 0);
		
		return ret;
	}
   
   explicit thread_intrusive_node(const db::node::node_id _id, const db::node::node_id _trans):
      thread_node(_id, _trans), prio_count(0)
   {
	}
   
   virtual ~thread_intrusive_node(void) {}
};

}

#endif
