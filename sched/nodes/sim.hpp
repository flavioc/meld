
#ifndef SCHED_NODES_SIM_HPP
#define SCHED_NODES_SIM_HPP

#include "mem/base.hpp"
#include "db/tuple.hpp"
#include "utils/spinlock.hpp"
#include "sched/base.hpp"
#include "queue/intrusive.hpp"
#include "queue/safe_simple_pqueue.hpp"
#include "queue/safe_linear_queue.hpp"

namespace sched
{ 

class sim_node: public db::node
{
public:
   vm::node_val top;
   vm::node_val bottom;
   vm::node_val east;
   vm::node_val west;
   vm::node_val north;
   vm::node_val south;

   vm::node_val *get_node_at_face(const int face) {
      switch(face) {
         case 0: return &bottom;
         case 1: return &north;
         case 2: return &east;
         case 3: return &west;
         case 4: return &south;
         case 5: return &top;
         default: assert(false);
      }
   }

   int get_face(const vm::node_val node) {
      if(node == bottom) return 0;
      if(node == north) return 1;
      if(node == east) return 2;
      if(node == west) return 3;
      if(node == south) return 4;
      if(node == top) return 5;
      return -1;
   }
	
	size_t timestamp;
   size_t neighbor_count;
	queue::heap_queue<db::simple_tuple*> tuple_pqueue; // for deterministic computation
	queue::push_safe_linear_queue<db::simple_tuple*> pending; // for fast computation

	inline bool has_work(void) const { return !tuple_pqueue.empty(); }

   virtual void assert_end(void) const
   {
      assert(!has_work());
   }

   virtual void assert_end_iteration(void) const
   {
      assert(!has_work());
   }

   explicit sim_node(const db::node::node_id _id, const db::node::node_id _trans, vm::all *all):
		db::node(_id, _trans, all), timestamp(0), neighbor_count(0)
   {
      top = bottom = west = east = north = south = -1;
		tuple_pqueue.set_type(HEAP_INT_ASC);
	}

   virtual ~sim_node(void) { }
};
   
}

#endif
