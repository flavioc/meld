
#ifndef SCHED_NODES_SERIAL_HPP
#define SCHED_NODES_SERIAL_HPP

#include "mem/base.hpp"
#include "sched/nodes/in_queue.hpp"
#include "sched/queue/bounded_pqueue.hpp"
#include "db/tuple.hpp"
#include "utils/spinlock.hpp"
#include "sched/base.hpp"

namespace sched
{ 

class serial_node: public in_queue_node
{
private:

   unsafe_bounded_pqueue<process::node_work>::type queue;

public:
   
   inline void add_work(process::node_work& new_work)
   {
      queue.push(new_work, new_work.get_strat_level());
   }
   
   inline bool has_work(void) const { return !queue.empty(); }

   inline process::node_work get_work(void)
   {
      assert(in_queue());
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

   explicit serial_node(const db::node::node_id _id, const db::node::node_id _trans):
      in_queue_node(_id, _trans),
      queue(vm::predicate::MAX_STRAT_LEVEL)
   {}

   virtual ~serial_node(void) { }
};
   
}

#endif