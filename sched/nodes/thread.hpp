
#ifndef SCHED_NODES_THREAD_HPP
#define SCHED_NODES_THREAD_HPP

#include "mem/base.hpp"
#include "sched/nodes/in_queue.hpp"
#include "sched/queue/bounded_pqueue.hpp"
#include "db/tuple.hpp"
#include "utils/spinlock.hpp"
#include "sched/base.hpp"
#include "process/work.hpp"

namespace sched
{

// forward declarations
class static_local; 
class static_local_prio;
class static_buff;
class dynamic_local;
class direct_local;
class threads_single;
class programmable_local;
class mpi_thread_dynamic;
class mpi_thread_static;
class mpi_thread_single;

class thread_node: public in_queue_node
{
private:
   
   friend class static_local;
   friend class static_local_prio;
   friend class static_buff;
   friend class dynamic_local;
   friend class direct_local;
   friend class threads_single;
   friend class programmable_local;
   friend class mpi_thread_static;
   friend class mpi_thread_dynamic;
   friend class mpi_thread_single;
   
   sched::base *owner;
	utils::spinlock spin;
   safe_bounded_pqueue<process::node_work>::type queue;
   
public:
   
   inline void set_owner(sched::base *_owner) { owner = _owner; }
   
   inline sched::base* get_owner(void) { return owner; }
   inline const sched::base* get_owner(void) const { return owner; }
   
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
   
   explicit thread_node(const db::node::node_id _id, const db::node::node_id _trans):
      in_queue_node(_id, _trans),
      owner(NULL),
      queue(vm::program::MAX_STRAT_LEVEL)
   {}
   
   virtual ~thread_node(void) { }
};

}

#endif
