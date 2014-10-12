
#ifndef SCHED_NODES_THREAD_INTRUSIVE_HPP
#define SCHED_NODES_THREAD_INTRUSIVE_HPP

#include "mem/base.hpp"
#include "queue/intrusive.hpp"
#include "queue/safe_simple_pqueue.hpp"
#include "sched/base.hpp"
#include "vm/state.hpp"

namespace sched
{

class thread_intrusive_node: public db::node
{
	DEFINE_PRIORITY_NODE(thread_intrusive_node);
	
private:

   // marker that indicates if the node should not be stolen.
   // when not NULL it indicates which scheduler it needs to be on.
   sched::base *static_node;
	
   double default_priority_level;
   double priority_level;
	
public:

	inline double get_priority_level(void) {
      if(priority_level == 0)
         return default_priority_level;
      return priority_level;
   }

	inline void set_priority_level(const double level) { priority_level = level; }
   inline void set_default_priority_level(const double level) { default_priority_level = level; }
	inline bool has_priority_level(void) const {
      return default_priority_level != 0.0 || priority_level != 0.0;
   }

   inline void set_static(sched::base *b) { static_node = b; }
   inline sched::base* get_static(void) const { return static_node; }
   inline void set_moving(void) { static_node = NULL; }
   inline bool is_static(void) const { return static_node != NULL; }
   inline bool is_moving(void) const { return static_node == NULL; }

   explicit thread_intrusive_node(const db::node::node_id _id, const db::node::node_id _trans):
		db::node(_id, _trans),
      INIT_PRIORITY_NODE(),
      static_node(NULL),
      default_priority_level(0.0),
      priority_level(0.0)
   {
	}
   
   virtual ~thread_intrusive_node(void) {}
};

}

#endif
