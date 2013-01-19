
#ifndef SCHED_NODES_IN_QUEUE_HPP
#define SCHED_NODES_IN_QUEUE_HPP

#include "mem/base.hpp"
#include "db/node.hpp"
#include "sched/base.hpp"

namespace sched
{

class in_queue_node: public db::node
{
protected:
   
   volatile bool i_am_on_queue;

public:
   
   inline void set_in_queue(const bool val)
   {
      assert(i_am_on_queue != val);
      i_am_on_queue = val;
   }
   
   inline bool in_queue(void) const
   {
      return i_am_on_queue;
   }

   virtual void assert_end(void) const
   {
      db::node::assert_end();
      assert(!in_queue());
   }

   virtual void assert_end_iteration(void) const
   {
      db::node::assert_end_iteration();
      assert(!in_queue());
   }

   explicit in_queue_node(const db::node::node_id _id, const db::node::node_id _trans, vm::all *all):
      db::node(_id, _trans, all),
      i_am_on_queue(false)
   {}

   virtual ~in_queue_node(void) { }
};
   
}

#endif
