
#ifndef SCHED_NODES_SERIAL_HPP
#define SCHED_NODES_SERIAL_HPP

#include "mem/base.hpp"
#include "sched/nodes/in_queue.hpp"
#include "db/tuple.hpp"
#include "utils/spinlock.hpp"
#include "sched/base.hpp"
#include "queue/intrusive.hpp"

namespace sched
{ 

class serial_node: public in_queue_node
{
public:
	
	DECLARE_DOUBLE_QUEUE_NODE(serial_node);

   virtual void assert_end(void) const
   {
      in_queue_node::assert_end();
      assert(!unprocessed_facts);
   }

   virtual void assert_end_iteration(void) const
   {
      in_queue_node::assert_end_iteration();
      assert(!unprocessed_facts);
   }

   explicit serial_node(const db::node::node_id _id, const db::node::node_id _trans, vm::all *all):
      in_queue_node(_id, _trans, all),
      INIT_DOUBLE_QUEUE_NODE()
   {}

   virtual ~serial_node(void) { }
};
   
}

#endif
