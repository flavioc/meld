
#ifndef SCHED_NODES_UNSAFE_STATIC_HPP
#define SCHED_NODES_UNSAFE_STATIC_HPP

#include "mem/base.hpp"
#include "sched/nodes/in_queue.hpp"
#include "queue/bounded_pqueue.hpp"
#include "db/tuple.hpp"
#include "sched/base.hpp"
#include "process/work.hpp"

namespace sched
{

// This node extends the in queue node with an unsafe queue
class unsafe_static_node: public in_queue_node
{
private:
   
	queue::unsafe_bounded_pqueue<process::node_work>::type queue;
   
public:
   
   inline void add_work(process::node_work& new_work)
   {
      queue.push(new_work, new_work.get_strat_level());
   }
   
   inline bool has_work(void) const { return !queue.empty(); }
   
   inline process::node_work get_work(void)
   {
      return queue.pop();
   }
   
   virtual void assert_end(void) const
   {
      in_queue_node::assert_end();
      assert(!has_work());
   }
   
   virtual void assert_end_iteration(void) const
   {
      in_queue_node::assert_end_iteration();
      assert(!has_work());
   }
   
   explicit unsafe_static_node(const db::node::node_id _id, const db::node::node_id _trans):
      in_queue_node(_id, _trans),
      queue(vm::program::MAX_STRAT_LEVEL)
   {}
   
   virtual ~unsafe_static_node(void) { }
};

}

#endif
