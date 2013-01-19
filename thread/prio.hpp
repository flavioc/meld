
#ifndef THREAD_STATIC_PRIO_HPP
#define THREAD_STATIC_PRIO_HPP

#include <boost/thread/barrier.hpp>
#include <vector>

#include "sched/base.hpp"
#include "thread/static.hpp"
#include "sched/nodes/thread_intrusive.hpp"
#include "queue/safe_double_queue.hpp"
#include "sched/thread/threaded.hpp"
#include "queue/safe_complex_pqueue.hpp"

//#define DO_ONE_PASS_FIRST

namespace sched
{

class threads_prio: public static_local
{
protected:
   
	typedef queue::intrusive_safe_complex_pqueue<thread_intrusive_node> priority_queue;

	priority_queue prio_queue;
   bool taken_from_priority_queue;

	heap_type priority_type;
	
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

   typedef std::map<db::node *, int, std::less<db::node *>,
           mem::allocator< std::pair<const db::node *, int> > > node_priorities;
   node_priorities node_map;

#ifdef DO_ONE_PASS_FIRST
   size_t to_takeout;
#endif

#ifdef TASK_STEALING
   virtual void check_stolen_nodes(void);
   virtual void answer_steal_requests(void);
#endif
	
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   bool set_next_node(void);
   bool check_if_current_useless();
   void make_active(void);
   void make_inactive(void);
   virtual void generate_aggs(void);
   virtual bool busy_wait(void);
	void add_prio_tuple(process::work, thread_intrusive_node *, db::simple_tuple *);
	void retrieve_prio_tuples(void);
   void check_priority_buffer(void);

	inline void add_to_priority_queue(thread_intrusive_node *node)
	{
		prio_queue.insert(node, node->get_priority_level());
	}
   
   virtual void add_to_queue(thread_intrusive_node *node)
   {
		if(node->has_priority_level())
			add_to_priority_queue(node);
		else
      	queue_nodes.push_tail(node);
   }
   
   virtual bool has_work(void) const { return static_local::has_work() || !prio_queue.empty() || !prio_tuples.empty(); }

public:
   
   virtual void init(const size_t);
   
   virtual void new_agg(process::work&);
   virtual void new_work(const db::node *, process::work&);
   virtual void new_work_other(sched::base *, process::work&);
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
   
   virtual db::node* get_work(void);
   virtual void finish_work(db::node *);
   virtual void end(void);
   virtual bool terminate_iteration(void);
   
   virtual void set_node_priority(db::node *, const int);
	virtual void add_node_priority(db::node *, const int);
   virtual void add_node_priority_other(db::node *, const int);
   virtual void set_node_priority_other(db::node *, const int);
   
   threads_prio *find_scheduler(const db::node *);
	virtual db::simple_tuple_vector gather_active_tuples(db::node *, const vm::predicate_id);
   virtual void gather_next_tuples(db::node *, db::simple_tuple_list&);
   
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
