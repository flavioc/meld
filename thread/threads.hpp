
#ifndef THREAD_THREADS_HPP
#define THREAD_THREADS_HPP

#include <vector>

#include "sched/base.hpp"
#include "queue/safe_linear_queue.hpp"
#include "thread/node.hpp"
#include "thread/termination_barrier.hpp"
#include "queue/safe_complex_pqueue.hpp"
#include "queue/safe_double_queue.hpp"
#include "utils/random.hpp"
#include "utils/circular_buffer.hpp"
#include "utils/tree_barrier.hpp"
#include "vm/bitmap.hpp"

#define DIRECT_PRIORITIES

namespace sched
{

#define STATE_IDLE queue_no_queue
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

class threads_sched: public sched::base
{
private:
   // thread state
   enum thread_state {
      THREAD_ACTIVE = 0,
      THREAD_INACTIVE = 1
   };

   std::atomic<thread_state> tstate;

   static termination_barrier *term_barrier;
   static utils::tree_barrier *thread_barrier;
#ifdef TASK_STEALING
   static std::atomic<int> *steal_states;
#endif
   
   utils::mutex lock;
	
#define threads_synchronize() thread_barrier->wait(get_id())

   inline void set_active(void)
   {
      assert(tstate == THREAD_INACTIVE);
      tstate = THREAD_ACTIVE;
#ifdef TASK_STEALING
      steal_states[get_id()].store(1, std::memory_order_release);
#endif
      term_barrier->is_active();
   }
   
   inline void set_inactive(void)
   {
      assert(tstate == THREAD_ACTIVE);
      tstate = THREAD_INACTIVE;
#ifdef TASK_STEALING
      steal_states[get_id()].store(0, std::memory_order_release);
#endif
      term_barrier->is_inactive();
   }

   inline void activate_thread(void)
   {
      LOCK_STACK(owner_lock);

      lock.lock(LOCK_STACK_USE(owner_lock));

      LOCK_STAT(sched_lock);

      if(is_inactive())
      {
         set_active();
         assert(is_active());
      }

      lock.unlock(LOCK_STACK_USE(owner_lock));
   }
   
   inline void set_active_if_inactive(void)
   {
      if(is_inactive())
         activate_thread();
   }
   
   static inline size_t num_active(void) { return term_barrier->num_active(); }
   
   inline bool is_inactive(void) const { return tstate == THREAD_INACTIVE; }
   inline bool is_active(void) const { return tstate == THREAD_ACTIVE; }
   
   static inline void reset_barrier(void) { term_barrier->reset(); }
   
   inline bool all_threads_finished(void) const
   {
      return term_barrier->all_finished();
   }
   
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

   thread_intrusive_node *current_node;
   vm::bitmap comm_threads; // threads we may need to communicate with

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
   size_t steal_nodes(thread_intrusive_node **, const size_t);

#ifdef INSTRUMENTATION
   size_t stolen_total = 0;
#endif

   void clear_steal_requests(void);
   bool go_steal_nodes(void);
#endif

#ifdef INSTRUMENTATION
   std::atomic<size_t> sent_facts_same_thread;
   std::atomic<size_t> sent_facts_other_thread;
   std::atomic<size_t> sent_facts_other_thread_now;
   // SET PRIORITY executed on nodes of the current thread.
   std::atomic<size_t> priority_nodes_thread;
   // SET PRIORITY executed on nodes of other threads.
   std::atomic<size_t> priority_nodes_others;
   // when locking nodes, how often did the lock fail.
   std::atomic<size_t> node_lock_fail;
   // when locking nodes, how often did the lock succeed.
   std::atomic<size_t> node_lock_ok;
#endif

#ifndef DIRECT_PRIORITIES
   char __padding[64];

   // priority buffer for adding priority requests
   // by other nodes.
#define PRIORITY_BUFFER_SIZE ((unsigned long)128)
   utils::circular_buffer<priority_add_item, PRIORITY_BUFFER_SIZE> priority_buffer;

   char __padding2[64];
   priority_add_item priority_tmp[PRIORITY_BUFFER_SIZE];

   void check_priority_buffer(void);
#endif

   // number of static nodes owned by this thread.
   std::atomic<uint64_t> static_nodes;

   inline void make_node_static(thread_intrusive_node *tn, threads_sched *target)
   {
      threads_sched *old(static_cast<threads_sched*>(tn->get_static()));

      if(old != NULL)
         old->static_nodes--;

      target->static_nodes++;
      tn->set_static(target);
   }

   inline void make_node_moving(thread_intrusive_node *tn)
   {
      threads_sched *old(static_cast<threads_sched*>(tn->get_static()));

      if(old != NULL)
         old->static_nodes--;

      tn->set_moving();
   }

   virtual void assert_end(void) const;
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
   void do_set_node_priority_other(thread_intrusive_node *, const double);

   virtual void killed_while_active(void);

   void do_set_node_priority(db::node *, const double);
   void add_node_priority_other(db::node *, const double);
   void set_node_priority_other(db::node *, const double);
   void set_node_owner(db::node *, threads_sched *);
   
public:

   virtual void init(const size_t);
   
   virtual void new_work(db::node *, db::node *, vm::tuple *, vm::predicate *, const vm::ref_count, const vm::depth_t);
   virtual void new_work_list(db::node *, db::node *, vm::tuple_array&);
#ifdef COMPILE_MPI
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
#endif
   
   virtual db::node* get_work(void);
   virtual void end(void);

   virtual void set_node_cpu(db::node *, const vm::int_val);
   virtual void set_node_static(db::node *);
   virtual void set_node_moving(db::node *);
   virtual void set_node_affinity(db::node *, db::node *);
   virtual void set_default_node_priority(db::node *, const double);
   virtual void set_node_priority(db::node *, const double);
	virtual void add_node_priority(db::node *, const double);
   virtual void schedule_next(db::node *);

   inline uint64_t num_static_nodes(void) const { return static_nodes; }

   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_intrusive_node(id, trans);
   }
   
   virtual void write_slice(statistics::slice&);

   static void init_barriers(const size_t);
   
   explicit threads_sched(const vm::process_id);
   
   virtual ~threads_sched(void);
};

}

#endif
