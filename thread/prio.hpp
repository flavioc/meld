
#ifndef THREAD_STATIC_PRIO_HPP
#define THREAD_STATIC_PRIO_HPP

#include <boost/thread/barrier.hpp>
#include <vector>

#include "sched/base.hpp"
#include "thread/threads.hpp"
#include "queue/safe_complex_pqueue.hpp"
#include "sched/nodes/thread_intrusive.hpp"
#include "queue/safe_double_queue.hpp"
#include "sched/thread/threaded.hpp"

namespace sched
{

extern db::database *prio_db;

class threads_prio: public threads_sched
{
protected:
   
	typedef queue::intrusive_safe_complex_pqueue<thread_intrusive_node> priority_queue;

	priority_queue prio_queue;
   bool taken_from_priority_queue;

	heap_type priority_type;
	
   typedef enum {
      ADD_PRIORITY,
      SET_PRIORITY
   } priority_add_type;

   typedef struct {
      priority_add_type typ;
      int val;
      db::node *target;
   } priority_add_item;

   // priority buffer
   queue::push_safe_linear_queue<priority_add_item> priority_buffer;

   typedef std::map<db::node *, int, std::less<db::node *>,
           mem::allocator< std::pair<const db::node *, int> > > node_priorities;
   node_priorities node_map;

#ifdef TASK_STEALING
   bool steal_flag;
   thread_intrusive_node *steal_node(void);
   virtual size_t number_of_nodes(void) const {
      return queue_nodes.size() + prio_queue.size();
   }
   virtual void check_stolen_node(thread_intrusive_node *);
#endif
	
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   bool set_next_node(void);
   bool check_if_current_useless();
   void check_priority_buffer(void);

	inline void add_to_priority_queue(thread_intrusive_node *node)
	{
		prio_queue.insert(node, node->get_priority_level());
	}
   
   virtual void add_to_queue(thread_intrusive_node *node)
   {
		if(node->has_priority_level()) {
         //cout << "Adding to prio\n";
			add_to_priority_queue(node);
      } else {
         //cout << "Adding to normal\n";
      	queue_nodes.push_tail(node);
      }
   }
   
   virtual bool has_work(void) const { return threads_sched::has_work() || !prio_queue.empty(); }
   void do_set_node_priority(db::node *, const double);

public:
   
   virtual void init(const size_t);
   
   virtual db::node* get_work(void);
   virtual void end(void);
   
   virtual void set_node_priority(db::node *, const double);
	virtual void add_node_priority(db::node *, const double);
   virtual void add_node_priority_other(db::node *, const double);
   virtual void set_node_priority_other(db::node *, const double);
   virtual void schedule_next(db::node *);
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans, vm::all *all)
   {
      return new thread_intrusive_node(id, trans, all);
   }
   
   DEFINE_START_FUNCTION(threads_prio);
   
   virtual void write_slice(statistics::slice&) const;
   
   explicit threads_prio(const vm::process_id, vm::all *);
   
   virtual ~threads_prio(void);
};

}

#endif
