
#ifndef SCHED_NODES_THREAD_HPP
#define SCHED_NODES_THREAD_HPP

#include "mem/base.hpp"
#include "sched/nodes/in_queue.hpp"
#include "db/tuple.hpp"
#include "sched/base.hpp"

namespace sched
{

class thread_node: public in_queue_node
{
public:
	   
   explicit thread_node(const db::node::node_id _id, const db::node::node_id _trans, vm::all *all):
      in_queue_node(_id, _trans, all)
   {}
   
   virtual ~thread_node(void) { }
};

}

#endif
