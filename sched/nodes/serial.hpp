
#ifndef SCHED_NODES_SERIAL_HPP
#define SCHED_NODES_SERIAL_HPP

#include "mem/base.hpp"
#include "db/node.hpp"
#include "sched/queue/bounded_pqueue.hpp"
#include "db/tuple.hpp"
#include "utils/spinlock.hpp"
#include "sched/base.hpp"

namespace sched
{

// forward declarations
class serial_local; 

class serial_node: public db::node
{
private:

   friend class serial_local;

   unsafe_bounded_pqueue<node_work_unit>::type queue;
   
   bool i_am_on_queue;

public:
   
   inline void add_work(const db::simple_tuple *tpl, const bool is_agg)
   {
      node_work_unit w = {tpl, is_agg};
      queue.push(w, tpl->get_strat_level());
   }
   
   inline void set_in_queue(const bool val)
   {
      assert(i_am_on_queue != val);
      i_am_on_queue = val;
   }
   
   inline bool in_queue(void) const
   {
      return i_am_on_queue;
   }

   inline bool has_work(void) const { return !queue.empty(); }

   inline node_work_unit get_work(void)
   {
      assert(in_queue());
      return queue.pop();
   }

   virtual void assert_end(void) const
   {
      db::node::assert_end();
      assert(!has_work());
      assert(!in_queue());
   }

   virtual void assert_end_iteration(void) const
   {
      assert(!has_work());
      assert(!in_queue());
   }

   explicit serial_node(const db::node::node_id _id, const db::node::node_id _trans):
      db::node(_id, _trans),
      queue(vm::predicate::MAX_STRAT_LEVEL),
      i_am_on_queue(false)
   {}

   virtual ~serial_node(void) { }
};
   
}

#endif