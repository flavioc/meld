
#ifndef SCHED_NODES_SIM_HPP
#define SCHED_NODES_SIM_HPP

#include <queue>

#include "mem/base.hpp"
#include "db/tuple.hpp"
#include "utils/spinlock.hpp"
#include "sched/base.hpp"
#include "queue/intrusive.hpp"
#include "queue/safe_simple_pqueue.hpp"
#include "queue/safe_linear_queue.hpp"
#include "utils/time.hpp"

namespace sched
{ 

enum face_t {
   BOTTOM = 0,
   NORTH,
   EAST,
   WEST,
   SOUTH,
   TOP
};

class sim_node: public db::node
{
private:
   boost::mutex delay_mtx;

   vm::node_val top;
   vm::node_val bottom;
   vm::node_val east;
   vm::node_val west;
   vm::node_val north;
   vm::node_val south;

   typedef struct {
      process::work work;
      // unix time in milliseconds
      utils::unix_timestamp when;
   } delay_work;

   struct delay_work_comparator {
      bool operator() (const delay_work& a, const delay_work& b) {
         return a.when < b.when;
      }
   };

   std::priority_queue<delay_work, std::vector<delay_work>, delay_work_comparator> delay_queue;

public:

   void add_delay_work(process::work& work, const vm::uint_val seconds)
   {
      delay_work w;

      w.work = work;
      w.when = utils::get_timestamp() + seconds * 1000;

      delay_mtx.lock();
      delay_queue.push(w);
      delay_mtx.unlock();
   }

   inline bool delayed_available(void) const
   {
      utils::unix_timestamp now(utils::get_timestamp());
      const delay_work& t(delay_queue.top());

      return t.when < now;
   }

   inline void add_delayed_tuples(db::simple_tuple_list& ls)
   {
      utils::unix_timestamp now(utils::get_timestamp());

      while(!delay_queue.empty()) {
         delay_mtx.lock();
         const delay_work& t(delay_queue.top());
         delay_mtx.unlock();

         if(t.when < now) {
            ls.push_back(t.work.get_tuple());
            delay_mtx.lock();
            delay_queue.pop();
            delay_mtx.unlock();
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

   int get_face(const vm::node_val node) {
      if(node == bottom) return 0;
      if(node == north) return 1;
      if(node == east) return 2;
      if(node == west) return 3;
      if(node == south) return 4;
      if(node == top) return 5;
      return -1;
   }
	
   /// XXX move this to private
	size_t timestamp;
   size_t neighbor_count;
	queue::heap_queue<db::simple_tuple*> tuple_pqueue; // for deterministic computation
	queue::heap_queue<db::simple_tuple*> rtuple_pqueue; // same as before but with retraction facts
	queue::push_safe_linear_queue<db::simple_tuple*> pending; // for fast computation

	inline bool has_work(void) const { return !tuple_pqueue.empty(); }

   inline void get_tuples_until_timestamp(db::simple_tuple_list& ls, const size_t until)
   {
      while(!tuple_pqueue.empty()) {
         heap_priority pr = tuple_pqueue.min_value();
         
         if((size_t)pr.int_priority > until)
            break;
         
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
