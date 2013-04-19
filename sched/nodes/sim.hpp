
#ifndef SCHED_NODES_SIM_HPP
#define SCHED_NODES_SIM_HPP

#include "mem/base.hpp"
#include "db/tuple.hpp"
#include "utils/spinlock.hpp"
#include "sched/base.hpp"
#include "queue/intrusive.hpp"
#include "queue/safe_simple_pqueue.hpp"
#include "queue/safe_linear_queue.hpp"
#include "utils/time.hpp"
#include "queue/safe_general_pqueue.hpp"

namespace sched
{ 

enum face_t {
   INVALID_FACE = -1,
   BOTTOM = 0,
   NORTH = 1,
   EAST = 2,
   WEST = 3,
   SOUTH = 4,
   TOP = 5
};

inline face_t& operator++(face_t &f)
{
   f = static_cast<face_t>(f + 1);
   return f;
}

inline face_t operator++(face_t& f, int) {
   ++f;
   return f;
}

class sim_node: public db::node
{
private:
   vm::node_val top;
   vm::node_val bottom;
   vm::node_val east;
   vm::node_val west;
   vm::node_val north;
   vm::node_val south;

   queue::general_pqueue<process::work, utils::unix_timestamp> delay_queue;

   bool instantiated_flag;
   size_t neighbor_count;

public:

   static const vm::node_val NO_NEIGHBOR = (vm::node_val)-1;

   static const face_t INITIAL_FACE = BOTTOM;
   static const face_t FINAL_FACE = TOP;

   void add_delay_work(process::work& work, const vm::uint_val milliseconds)
   {
      delay_queue.push(work, utils::get_timestamp() + milliseconds);
   }

   inline bool delayed_available(void) const
   {
      if(delay_queue.empty())
         return false;

      utils::unix_timestamp now(utils::get_timestamp());
      utils::unix_timestamp best(delay_queue.top_priority());

      return best < now;
   }

   inline void add_delayed_tuples(db::simple_tuple_list& ls)
   {
      utils::unix_timestamp now(utils::get_timestamp());

      while(!delay_queue.empty()) {
         const utils::unix_timestamp best(delay_queue.top_priority());

         if(best < now) {
            ls.push_back(delay_queue.pop().get_tuple());
         } else
            break;
      }
   }

   // returns a pointer to a certain face, allowing modification
   vm::node_val *get_node_at_face(const face_t face) {
      switch(face) {
         case BOTTOM: return &bottom;
         case NORTH: return &north;
         case EAST: return &east;
         case WEST: return &west;
         case SOUTH: return &south;
         case TOP: return &top;
         default: assert(false);
      }
   }

   face_t get_face(const vm::node_val node) {
      if(node == bottom) return BOTTOM;
      if(node == north) return NORTH;
      if(node == east) return EAST;
      if(node == west) return WEST;
      if(node == south) return SOUTH;
      if(node == top) return TOP;
      return INVALID_FACE;
   }
	
   /// XXX move this to private
   vm::deterministic_timestamp timestamp;
	queue::heap_queue<db::simple_tuple*> tuple_pqueue; // for deterministic computation
	queue::heap_queue<db::simple_tuple*> rtuple_pqueue; // same as before but with retraction facts
	queue::push_safe_linear_queue<db::simple_tuple*> pending; // for threaded computation

	inline bool has_work(void) const { return !tuple_pqueue.empty(); }

   inline void get_tuples_until_timestamp(db::simple_tuple_list& ls, const size_t until)
   {
      while(!tuple_pqueue.empty()) {
         heap_priority pr = tuple_pqueue.min_value();
         
         if((size_t)pr.int_priority > until) {
            break;
         }
         
         db::simple_tuple *stpl(tuple_pqueue.pop());
         ls.push_back(stpl);
      }

      while(!rtuple_pqueue.empty()) {
         heap_priority pr = rtuple_pqueue.min_value();
         if((size_t)pr.int_priority > until)
            break;
         
         db::simple_tuple *stpl(rtuple_pqueue.pop());
         ls.push_back(stpl);
      }
   }

   virtual void assert_end(void) const
   {
      assert(!has_work());
   }

   virtual void assert_end_iteration(void) const
   {
      assert(!has_work());
   }

   inline bool has_been_instantiated(void) const
   {
      return instantiated_flag;
   }

   inline void set_instantiated(const bool flag)
   {
      instantiated_flag = flag;
   }

   inline void inc_neighbor_count(void)
   {
      ++neighbor_count;
   }

   inline void dec_neighbor_count(void)
   {
      --neighbor_count;
   }

   inline size_t get_neighbor_count(void) const
   {
      return neighbor_count;
   }

   explicit sim_node(const db::node::node_id _id, const db::node::node_id _trans, vm::all *all):
		db::node(_id, _trans, all),
      top(NO_NEIGHBOR), bottom(NO_NEIGHBOR), east(NO_NEIGHBOR),
      west(NO_NEIGHBOR), north(NO_NEIGHBOR), south(NO_NEIGHBOR),
      instantiated_flag(false),
      neighbor_count(0),
      timestamp(0)
   {
      top = bottom = west = east = north = south = -1;
		tuple_pqueue.set_type(HEAP_INT_ASC);
	}

   virtual ~sim_node(void) { }
};
   
}

#endif
