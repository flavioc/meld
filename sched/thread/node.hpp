
#ifndef SCHED_THREAD_NODE_HPP
#define SCHED_THREAD_NODE_HPP

#include "mem/base.hpp"
#include "db/node.hpp"
#include "sched/queue/bounded_pqueue.hpp"
#include "db/tuple.hpp"
#include "utils/spinlock.hpp"

namespace sched
{

// forward declarations
class static_local; 
class dynamic_local;
class mpi_thread_dynamic;
class mpi_thread_static;
class threads_single;

struct node_work_unit {
   const db::simple_tuple *work_tpl;
   bool agg;
};

class thread_node: public db::node
{
private:
   
   friend class static_local;
   friend class dynamic_local;
   friend class mpi_thread_static;
   friend class mpi_thread_dynamic;
   friend class threads_single;
   
   static_local *owner;
   volatile bool i_am_on_queue;
	utils::spinlock spin;
   safe_bounded_pqueue<node_work_unit>::type queue;
   
public:
   
   inline void set_owner(static_local *_owner) { owner = _owner; }
   
   inline const bool in_queue(void) const { return i_am_on_queue; }
   
   inline void set_in_queue(const bool new_val) {
      assert(i_am_on_queue != new_val);
      i_am_on_queue = new_val;
   }
   
   inline static_local* get_owner(void) { return owner; }
   
   inline void add_work(const db::simple_tuple *tpl, const bool is_agg = false)
   {
      node_work_unit w = {tpl, is_agg};
      queue.push(w, tpl->get_strat_level());
   }
   
   inline const bool no_more_work(void) const { return queue.empty(); }
   
   inline node_work_unit get_work(void)
   {
      return queue.pop();
   }
   
   virtual void assert_end(void) const
   {
      db::node::assert_end();
      assert(queue.empty());
   }
   
   virtual void assert_end_iteration(void) const
   {
      assert(queue.empty());
   }
   
   explicit thread_node(const db::node::node_id _id, const db::node::node_id _trans):
      db::node(_id, _trans),
      owner(NULL),
      i_am_on_queue(false), // must be false! node will be added to queue automatically
      queue(vm::predicate::MAX_STRAT_LEVEL)
   {}
   
   virtual ~thread_node(void) { }
};

}

#endif
