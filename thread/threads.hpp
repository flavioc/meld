
#ifndef THREAD_THREADS_HPP
#define THREAD_THREADS_HPP

#include <vector>

#include "db/database.hpp"
#include "thread/ids.hpp"
#include "queue/safe_linear_queue.hpp"
#include "thread/termination_barrier.hpp"
#include "queue/safe_complex_pqueue.hpp"
#include "queue/safe_double_queue.hpp"
#include "utils/random.hpp"
#include "utils/circular_buffer.hpp"
#include "utils/tree_barrier.hpp"
#include "vm/bitmap.hpp"
#ifdef INSTRUMENTATION
#include "stat/stat.hpp"
#include "stat/slice.hpp"
#endif

#define DIRECT_PRIORITIES

//#define DEBUG_PRIORITIES
//#define PROFILE_QUEUE
#define SEND_OTHERS
#ifdef DIRECT_PRIORITIES
#undef SEND_OTHERS
#endif

// node stealing strategies.
//#define STEAL_ONE
#define STEAL_HALF

#ifdef INSTRUMENTATION
#define NODE_LOCK(NODE, ARG, STAT) if((NODE)->main_lock.try_lock1(LOCK_STACK_USE(ARG))) { node_lock_ok++; } else { node_lock_fail++; (NODE)->main_lock.lock1(LOCK_STACK_USE(ARG)); }
#else
#define NODE_LOCK(NODE, ARG, STAT) MUTEX_LOCK((NODE)->main_lock, ARG, STAT)
#endif
#define NODE_UNLOCK(NODE, ARG) MUTEX_UNLOCK((NODE)->main_lock, ARG)

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
   vm::priority_t val;
   db::node *target;
} priority_add_item;

class threads_sched: public mem::base
{
private:
   
   const vm::process_id id;

   char __padding_state[128];
   
   vm::state state;

   ids node_handler;
   
#ifdef INSTRUMENTATION
   mutable std::atomic<statistics::sched_state> ins_state{statistics::NOW_ACTIVE};
   
#define ins_active ins_state = statistics::NOW_ACTIVE
#define ins_idle ins_state = statistics::NOW_IDLE
#define ins_sched ins_state = statistics::NOW_SCHED
#define ins_round ins_state = statistics::NOW_ROUND

#else

#define ins_active
#define ins_idle
#define ins_sched
#define ins_round

#endif

   // thread state
   enum thread_state {
      THREAD_ACTIVE = 0,
      THREAD_INACTIVE = 1
   };

   std::atomic<thread_state> tstate{THREAD_ACTIVE};

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
      MUTEX_LOCK_GUARD(lock, thread_lock);

      if(is_inactive())
      {
         set_active();
         assert(is_active());
      }
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

   void do_loop(void);
   
   using node_queue = queue::intrusive_safe_double_queue<db::node>;
   struct Queues {
      node_queue moving;
      node_queue stati;

      inline bool has_work(void) const
      {
         return !moving.empty() || !stati.empty();
      }

      explicit Queues(void):
         moving(NORMAL_QUEUE_MOVING),
         stati(NORMAL_QUEUE_STATIC)
      {
      }
   } queues;

	using priority_queue = queue::intrusive_safe_complex_pqueue<db::node>;

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

	inline void add_to_priority_queue(db::node *node)
	{
      if(node->is_static())
         prios.stati.insert(node, node->get_priority());
      else
         prios.moving.insert(node, node->get_priority());
	}

   db::node *current_node{nullptr};
   vm::bitmap comm_threads; // threads we may need to communicate with

   inline bool pop_node_from_queues(void)
   {
      if(queues.stati.pop_head(current_node, STATE_WORKING))
         return true;
      return queues.moving.pop_head(current_node, STATE_WORKING);
   }

#ifdef TASK_STEALING
   mutable utils::randgen rand;
   size_t next_thread;
   size_t backoff;
   bool steal_flag;
   size_t steal_nodes(db::node **, const size_t);

#ifdef INSTRUMENTATION
   std::atomic<size_t> stolen_total{0};
#endif

   void clear_steal_requests(void);
   bool go_steal_nodes(void);
#endif

#ifdef INSTRUMENTATION
   std::atomic<size_t> sent_facts_same_thread{0};
   std::atomic<size_t> sent_facts_other_thread{0};
   std::atomic<size_t> sent_facts_other_thread_now{0};
   // SET PRIORITY executed on nodes of the current thread.
   std::atomic<size_t> priority_nodes_thread{0};
   // SET PRIORITY executed on nodes of other threads.
   std::atomic<size_t> priority_nodes_others{0};
   std::atomic<size_t> node_lock_ok{0};
   std::atomic<size_t> node_lock_fail{0};
   std::atomic<size_t> thread_transactions{0};
   std::atomic<size_t> all_transactions{0};
   db::node::node_id last_node{0};
   std::atomic<int32_t> node_difference{0};
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
   std::atomic<uint64_t> static_nodes{0};

   inline void make_node_static(db::node *tn, threads_sched *target)
   {
      threads_sched *old(tn->get_static());

      if(old != nullptr)
         old->static_nodes--;

      target->static_nodes++;
      tn->set_static(target);
   }

   inline void make_node_moving(db::node *tn)
   {
      threads_sched *old(tn->get_static());

      if(old != nullptr)
         old->static_nodes--;

      tn->set_moving();
   }

   void assert_end(void) const;
   bool set_next_node(void);
   void make_current_node_inactive(void);
   bool check_if_current_useless();
   void make_active(void);
   void make_inactive(void);
   bool busy_wait(void);
   
   void add_to_queue(db::node *node)
   {
		if(node->has_priority())
			add_to_priority_queue(node);
      else {
         if(node->is_static())
            queues.stati.push_tail(node);
         else
            queues.moving.push_tail(node);
      }
   }
   
   bool has_work(void) const
   {
      return queues.has_work() || prios.has_work();
   }

   void move_node_to_new_owner(db::node *, threads_sched *);
   void do_set_node_priority_other(db::node *, const vm::priority_t);
   void do_remove_node_priority_other(db::node *);

   void killed_while_active(void);

   void do_set_node_priority(db::node *, const vm::priority_t);
   void do_remove_node_priority(db::node *);
   void add_node_priority_other(db::node *, const vm::priority_t);
   void set_node_priority_other(db::node *, const vm::priority_t);
   void set_node_owner(db::node *, threads_sched *);

   inline void setup_node(db::node *node)
   {
      vm::predicate *init_pred(vm::theProgram->get_init_predicate());
      vm::tuple *init_tuple(vm::tuple::create(init_pred));
#ifdef FACT_STATISTICS
      state.facts_derived++;
#endif
      node->set_owner(this);
      node->add_linear_fact(init_tuple, init_pred);
      node->unprocessed_facts = true;
   }
   
public:

   // allows the core machine to stop threads
   static std::atomic<bool> stop_flag;

   inline vm::process_id get_id(void) const { return id; }
   
   inline bool leader_thread(void) const { return get_id() == 0; }

   size_t queue_size() const {
      return queues.stati.size() + queues.moving.size() + prios.stati.size() + prios.moving.size();
   }

   db::node* init_node(db::database::map_nodes::iterator it)
   {
      db::node *node(vm::All->DATABASE->create_node_iterator(it));
#ifdef USE_REAL_NODES
      vm::theProgram->fix_node_address(node);
#endif
#ifdef GC_NODES
      // initial nodes never get deleted.
      node->refs++;
#endif
      setup_node(node);
      return node;
   }

   inline db::node* create_node(void)
   {
      db::node *node(node_handler.create_node());
      setup_node(node);
      return node;
   }

   inline void delete_node(db::node *node)
   {
      node_handler.delete_node(node);
   }

   inline void merge_new_nodes(threads_sched& b)
   {
      node_handler.merge(b.node_handler);
   }

   inline void commit_nodes(void)
   {
      node_handler.commit();
   }

   void loop(void);
   void init(const size_t);
   
   void new_work(db::node *, db::node *, vm::tuple *, vm::predicate *, const vm::derivation_direction, const vm::depth_t);
   void new_work_list(db::node *, db::node *, vm::tuple_array&);
   void new_work_delay(db::node *, db::node *, vm::tuple*, vm::predicate *,
         const vm::derivation_direction, const vm::depth_t, const vm::uint_val)
   {
      assert(false);
   }
   
   db::node* get_work(void);
   void end(void);

   void set_node_cpu(db::node *, const vm::int_val);
   void set_node_static(db::node *);
   void set_node_moving(db::node *);
   void set_node_affinity(db::node *, db::node *);
   void set_default_node_priority(db::node *, const vm::priority_t);
   void set_node_priority(db::node *, const vm::priority_t);
	void add_node_priority(db::node *, const vm::priority_t);
   void remove_node_priority(db::node *);
   void schedule_next(db::node *);

   inline uint64_t num_static_nodes(void) const { return static_nodes; }
#ifdef LOCK_STATISTICS
   void print_average_priority_size(void) const;
#endif

#ifdef FACT_STATISTICS
   vm::state& get_state(void) { return state; }

   uint64_t count_add_work_self = 0;
   uint64_t count_add_work_other = 0;
   uint64_t count_stolen_nodes = 0;
   uint64_t count_set_priority = 0;
   uint64_t count_add_priority = 0;
#endif
#ifdef INSTRUMENTATION
   void write_slice(statistics::slice&);
#endif

   static void init_barriers(const size_t);
   
   explicit threads_sched(const vm::process_id);
   
   ~threads_sched(void);
};

}

#endif
