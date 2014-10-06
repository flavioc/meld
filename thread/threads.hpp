
#ifndef THREAD_THREADS_HPP
#define THREAD_THREADS_HPP

#include <boost/thread/barrier.hpp>
#include <vector>

#include "sched/base.hpp"
#include "queue/safe_linear_queue.hpp"
#include "sched/thread/threaded.hpp"
#include "sched/nodes/thread_intrusive.hpp"
#include "queue/safe_complex_pqueue.hpp"
#include "queue/safe_double_queue.hpp"
#include "utils/random.hpp"
#include "utils/circular_buffer.hpp"

namespace sched
{

#define STATE_WORKING 0
#define STATE_STEALING 1
#define STATE_PRIO_CHANGE 2
#define STATE_STATIC_CHANGE 3
#define STATE_AFFINITY_CHANGE 4
#define NORMAL_QUEUE_MOVING 100
#define NORMAL_QUEUE_STATIC 101
#define PRIORITY_MOVING 102
#define PRIORITY_STATIC 103

typedef enum {
   ADD_PRIORITY,
   SET_PRIORITY
} priority_add_type;

typedef struct {
   priority_add_type typ;
   int val;
   db::node *target;
} priority_add_item;

class threads_sched: public sched::base,
                    public sched::threaded
{
protected:
   
   typedef queue::intrusive_safe_double_queue<thread_intrusive_node> node_queue;
   struct Queues {
      node_queue moving;
      node_queue stati;
      bool use_static;

      inline bool has_work(void) const
      {
         return !moving.empty() || !stati.empty();
      }

      explicit Queues(void):
         moving(NORMAL_QUEUE_MOVING),
         stati(NORMAL_QUEUE_STATIC),
         use_static(false)
      {
      }
   } queues;

   inline bool node_in_normal_queue(thread_intrusive_node *tn) const
   {
      return tn->node_state() == NORMAL_QUEUE_MOVING || tn->node_state() == NORMAL_QUEUE_STATIC;
   }

	typedef queue::intrusive_safe_complex_pqueue<thread_intrusive_node> priority_queue;

   struct Priorities {
      priority_queue moving;
      priority_queue stati;

      inline bool has_work(void) const {
         return !moving.empty() || !stati.empty();
      }

      explicit Priorities(void):
         moving(PRIORITY_MOVING),
         stati(PRIORITY_STATIC)
      {
      }
   } prios;

	inline void add_to_priority_queue(thread_intrusive_node *node)
	{
      if(node->is_static())
         prios.stati.insert(node, node->get_priority_level());
      else
         prios.moving.insert(node, node->get_priority_level());
	}

   inline bool node_in_priority_queue(thread_intrusive_node *tn) const
   {
      return tn->node_state() == PRIORITY_MOVING || tn->node_state() == PRIORITY_STATIC;
   }

   thread_intrusive_node *current_node;

   inline bool pop_node_from_queues(void)
   {
      queues.use_static = !queues.use_static;
      if(queues.use_static) {
         if(queues.stati.pop_head(current_node, STATE_WORKING))
            return true;
         if(queues.moving.pop_head(current_node, STATE_WORKING))
            return true;
      } else {
         if(queues.moving.pop_head(current_node, STATE_WORKING))
            return true;
         if(queues.stati.pop_head(current_node, STATE_WORKING))
            return true;
      }
     
      return false;
   }

#ifdef TASK_STEALING
   mutable utils::randgen rand;
   size_t next_thread;
   size_t backoff;
   bool steal_flag;
   thread_intrusive_node *steal_node(void);

#ifdef INSTRUMENTATION
   size_t stolen_total;
#endif

   void clear_steal_requests(void);
   bool go_steal_nodes(void);
#endif

#ifdef INSTRUMENTATION
   size_t sent_facts_same_thread;
   size_t sent_facts_other_thread;
   size_t sent_facts_other_thread_now;
#endif

   char __padding[64];

   // priority buffer for adding priority requests
   // by other nodes.
#define PRIORITY_BUFFER_SIZE ((unsigned long)128)
   utils::circular_buffer<priority_add_item, PRIORITY_BUFFER_SIZE> priority_buffer;

   void check_priority_buffer(void);

   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   bool set_next_node(void);
   virtual bool check_if_current_useless();
   void make_active(void);
   void make_inactive(void);
   virtual void generate_aggs(void);
   virtual bool busy_wait(void);
   
   void add_to_queue(thread_intrusive_node *node)
   {
		if(node->has_priority_level())
			add_to_priority_queue(node);
      else {
         if(node->is_static())
            queues.stati.push_tail(node);
         else
            queues.moving.push_tail(node);
      }
   }
   
   virtual bool has_work(void) const
   {
      return queues.has_work() || prios.has_work();
   }

   void move_node_to_new_owner(thread_intrusive_node *, threads_sched *);

   virtual void killed_while_active(void);

   void do_set_node_priority(db::node *, const double);
   void add_node_priority_other(db::node *, const double);
   void set_node_priority_other(db::node *, const double);
   
public:
   
   virtual void init(const size_t);
   
   virtual void new_agg(process::work&);
   virtual void new_work(db::node *, db::node *, vm::tuple *, vm::predicate *, const vm::ref_count, const vm::depth_t);
#ifdef COMPILE_MPI
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
#endif
   
   virtual db::node* get_work(void);
   virtual void end(void);
   virtual bool terminate_iteration(void);

   virtual void set_node_static(db::node *);
   virtual void set_node_moving(db::node *);
   virtual void set_node_affinity(db::node *, db::node *);
   virtual void set_default_node_priority(db::node *, const double);
   virtual void set_node_priority(db::node *, const double);
	virtual void add_node_priority(db::node *, const double);
   virtual void schedule_next(db::node *);
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_intrusive_node(id, trans);
   }
   
   virtual void write_slice(statistics::slice&);
   
   explicit threads_sched(const vm::process_id);
   
   virtual ~threads_sched(void);
};

}

#endif
