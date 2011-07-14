
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

   unsafe_bounded_pqueue<node_work_unit>::type queue;

public:
   
   inline void add_work(const db::simple_tuple *tpl, const bool is_agg)
   {
      node_work_unit w = {tpl, is_agg};
      queue.push(w, tpl->get_strat_level());
   }
   
   inline bool has_work(void) const { return !queue.empty(); }

   inline node_work_unit get_work(void)
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