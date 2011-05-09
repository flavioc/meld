
#ifndef SCHED_NODE_HPP
#define SCHED_NODE_HPP

#include "mem/base.hpp"
#include "db/node.hpp"
#include "sched/queue.hpp"
#include "db/tuple.hpp"

namespace sched
{

class stealer; // forward declaration

struct node_work_unit {
   const db::simple_tuple *work_tpl;
   bool agg;
};

class thread_node: public db::node
{
private:
   
   friend class stealer;
   
   stealer *owner;
   bool i_am_on_queue;
   boost::mutex mtx;
   safe_queue<node_work_unit> queue;
   
public:
   
   inline void set_owner(stealer *_owner) { owner = _owner; }
   inline const bool in_queue(void) const { return i_am_on_queue; }
   inline void set_in_queue(const bool new_val) { i_am_on_queue = new_val; }
   inline stealer* get_owner(void) { return owner; }
   
   inline void add_work(const db::simple_tuple *tpl, const bool is_agg = false) {
      node_work_unit w = {tpl, is_agg};
      queue.push(w);
   }
   
   inline const bool no_more_work(void) const { return queue.empty(); }
   
   inline node_work_unit get_work(void)
   {
      return queue.pop();
   }
   
   explicit thread_node(const db::node::node_id _id, const db::node::node_id _trans):
      db::node(_id, _trans),
      owner(NULL),
      i_am_on_queue(false) // must be false! node will be added to queue automatically
   {}
   
   virtual ~thread_node(void) { }
};

}

#endif
