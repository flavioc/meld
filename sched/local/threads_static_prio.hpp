
#ifndef SCHED_LOCAL_THREADS_STATIC_PRIO_HPP
#define SCHED_LOCAL_THREADS_STATIC_PRIO_HPP

#include <boost/thread/barrier.hpp>
#include <vector>

#include "sched/base.hpp"
#include "sched/nodes/thread_intrusive.hpp"
#include "queue/safe_double_queue.hpp"
#include "sched/thread/threaded.hpp"
#include "sched/thread/queue_buffer.hpp"

//#define PRIO_NORMAL
//#define PRIO_HEAD
#define PRIO_OPT
//#define PRIO_INVERSE

namespace sched
{

class static_local_prio: public sched::base,
                    public sched::threaded
{
protected:
   
   DEFINE_PADDING;
   
   typedef queue::intrusive_safe_double_queue<thread_intrusive_node> queue;
   queue queue_nodes;

	DEFINE_PADDING;
	
	queue prio_queue;
   
   DEFINE_PADDING;
   
   thread_intrusive_node *current_node;
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   bool set_next_node(void);
   bool check_if_current_useless();
   void make_active(void);
   void make_inactive(void);
   virtual void generate_aggs(void);
   virtual bool busy_wait(void);
   
   inline void add_to_queue(thread_intrusive_node *node)
   {
#ifdef PRIO_NORMAL
		queue_nodes.push_tail(node);
#elif defined(PRIO_OPT) || defined(PRIO_INVERSE)
		if(node->with_priority()) {
			prio_queue.push_tail(node);
			node->unmark_priority();
			node->set_is_in_prioqueue(true);
		} else {
			node->set_is_in_prioqueue(false);
      	queue_nodes.push_tail(node);
		}
#elif defined(PRIO_HEAD)
		queue_nodes.push_head(node);
#endif
   }
   
   inline bool has_work(void) const { return !queue_nodes.empty() || !prio_queue.empty(); }

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
   
   static_local_prio *find_scheduler(const db::node *);
   
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
