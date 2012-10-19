
#ifndef THREAD_STATIC_PRIO_HPP
#define THREAD_STATIC_PRIO_HPP

#include <boost/thread/barrier.hpp>
#include <vector>

#include "sched/base.hpp"
#include "sched/nodes/thread_intrusive.hpp"
#include "queue/safe_double_queue.hpp"
#include "sched/thread/threaded.hpp"
#include "queue/safe_complex_pqueue.hpp"

#define DO_ONE_PASS_FIRST

namespace sched
{

class static_local_prio: public sched::base,
                    public sched::threaded
{
protected:
   
   DEFINE_PADDING;
   
   typedef queue::intrusive_safe_double_queue<thread_intrusive_node> node_queue;
   node_queue queue_nodes;

	typedef queue::intrusive_safe_complex_pqueue<thread_intrusive_node> priority_queue;

	DEFINE_PADDING;
	
	priority_queue prio_queue;
   
   DEFINE_PADDING;
   
   thread_intrusive_node *current_node;
   bool taken_from_priority_queue;

	heap_type priority_type;
	
	// buffer
	queue::push_safe_linear_queue<process::work> prio_tuples;

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

#ifdef DO_ONE_PASS_FIRST
   size_t to_takeout;
#endif
	
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   bool set_next_node(void);
   bool check_if_current_useless();
   void make_active(void);
   void make_inactive(void);
   virtual void generate_aggs(void);
   virtual bool busy_wait(void);
	void add_prio_tuple(process::node_work, thread_intrusive_node *, db::simple_tuple *);
	void retrieve_prio_tuples(void);
   void check_priority_buffer(void);

	inline void add_to_priority_queue(thread_intrusive_node *node)
	{
		heap_priority pr;
		pr.int_priority = node->get_priority_level();
		prio_queue.insert(node, pr);
		node->unmark_priority();
	}
   
   inline void add_to_queue(thread_intrusive_node *node)
   {
		if(node->with_priority())
			add_to_priority_queue(node);
		else
      	queue_nodes.push_tail(node);
   }
   
   inline bool has_work(void) const { return !queue_nodes.empty() || !prio_queue.empty() || !prio_tuples.empty(); }

public:
   
   virtual void init(const size_t);
   
   virtual void new_agg(process::work&);
   virtual void new_work(const db::node *, process::work&);
   virtual void new_work_other(sched::base *, process::work&);
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
   
   virtual bool get_work(process::work&);
   virtual void finish_work(const process::work&);
   virtual void end(void);
   virtual bool terminate_iteration(void);
   
   virtual void set_node_priority(db::node *, const int);
	virtual void add_node_priority(db::node *, const int);
   virtual void add_node_priority_other(db::node *, const int);
   virtual void set_node_priority_other(db::node *, const int);
   
   static_local_prio *find_scheduler(const db::node *);
	virtual db::simple_tuple_list gather_active_tuples(db::node *, const vm::predicate_id);
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_intrusive_node(id, trans);
   }
   
   static std::vector<sched::base*>& start(const size_t);
   
   virtual void write_slice(statistics::slice&) const;
   
   explicit static_local_prio(const vm::process_id);
   
   virtual ~static_local_prio(void);
};

}

#endif
