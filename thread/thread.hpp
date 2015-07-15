
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
//#include "thread/priority_queue.hpp"
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

static inline bool
is_higher_priority(const vm::priority_t priority, db::node *tn)
{
   return vm::higher_priority(priority, tn->get_priority());
}


class thread: public mem::base
{
private:
   
   const vm::process_id id;
   char __pad[1024];

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

#define THREAD_ACTIVE (utils::byte)0x1
#define THREAD_INACTIVE (utils::byte)0x2
#define THREAD_NEW_WORK (utils::byte)0x4

   volatile utils::byte tstate{THREAD_ACTIVE};

   static termination_barrier *term_barrier;
   static utils::tree_barrier *thread_barrier;
   
#define threads_synchronize() thread_barrier->wait(get_id())
public:

   inline void activate_thread(void)
   {
      utils::byte active_work(THREAD_ACTIVE | THREAD_NEW_WORK);
      while(true) {
         utils::byte old_val(tstate);
         if(old_val == THREAD_INACTIVE) {
            if(cmpxchg(&tstate, old_val, active_work) == old_val) {
               term_barrier->is_active();
               return;
            }
         } else if(old_val == THREAD_ACTIVE) {
            if(cmpxchg(&tstate, old_val, active_work) == old_val)
               return;
         } else if(old_val == (THREAD_ACTIVE | THREAD_NEW_WORK)) {
            return;
         }
         cpu_relax();
      }
   }
   
   inline bool set_active_if_inactive(void)
   {
      utils::byte active(THREAD_ACTIVE);
      while(true) {
         utils::byte old_val(tstate);
         if(old_val == THREAD_INACTIVE) {
            if(cmpxchg(&tstate, old_val, active) == old_val) {
               term_barrier->is_active();
               return false;
            }
         } else if(old_val == THREAD_ACTIVE) {
            return false;
         } else if(old_val == (THREAD_ACTIVE | THREAD_NEW_WORK)) {
            if(cmpxchg(&tstate, old_val, active) == old_val)
               return true;
         }
         cpu_relax();
      }
   }

   inline bool set_inactive_if_no_work()
   {
      utils::byte active(THREAD_ACTIVE);
      utils::byte inactive(THREAD_INACTIVE);
      // return true if there is new work
      while(true) {
         utils::byte old_val(tstate);
         if(old_val == inactive)
            return false;
         else if(old_val == THREAD_ACTIVE) {
            if(cmpxchg(&tstate, old_val, inactive) == old_val) {
               term_barrier->is_inactive();
               return false;
            }
         } else if(old_val == (THREAD_ACTIVE | THREAD_NEW_WORK)) {
            if(cmpxchg(&tstate, old_val, active) == old_val)
               return true;
         }
         cpu_relax();
      }
   }

   inline void set_force_inactive()
   {
      utils::byte active(THREAD_ACTIVE);
      utils::byte inactive(THREAD_INACTIVE);
      while(true) {
         utils::byte old_val(tstate);
         if(old_val == inactive)
            return;
         else if(old_val == active) {
            if(cmpxchg(&tstate, old_val, inactive) == old_val) {
               term_barrier->is_inactive();
               return;
            }
         } else if(old_val == (active | THREAD_NEW_WORK)) {
            if(cmpxchg(&tstate, old_val, inactive) == old_val)
               return;
         }
         cpu_relax();
      }
   }

   static inline size_t num_active(void) { return term_barrier->num_active(); }
   
   inline bool is_inactive() const { return tstate == THREAD_INACTIVE; }
   inline bool is_active() const { return tstate & THREAD_ACTIVE; }
   
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
  // using priority_queue = sched::priority_queue;

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

   mutable utils::randgen rand;
#ifdef TASK_STEALING
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

   inline void make_node_static(db::node *tn, thread *target)
   {
      assert(target);

      thread *old(tn->get_static());

      if(old != nullptr)
         old->static_nodes--;

      target->static_nodes++;
      tn->set_static(target);
   }

   inline void make_node_moving(db::node *tn)
   {
      thread *old(tn->get_static());

      if(old != nullptr)
         old->static_nodes--;

      tn->set_moving();
   }

   void assert_end(void) const;
   inline bool set_next_node(void)  __attribute__((always_inline));
   void make_current_node_inactive(void);
   inline bool check_if_current_useless();
   void make_active(void);
   void make_inactive(void);
   bool busy_wait(void);
   
   inline void add_to_queue(db::node *node) __attribute__((always_inline))
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

   void move_node_to_new_owner(db::node *, thread *);
   void do_set_node_priority_other(db::node *, const vm::priority_t);
   void do_remove_node_priority_other(db::node *);

   void killed_while_active(void);

   void do_remove_node_priority(db::node *);
   void add_node_priority_other(db::node *, const vm::priority_t);
   void set_node_priority_other(db::node *, const vm::priority_t);
   void set_node_owner(db::node *, thread *);

   void setup_thread_node();

   inline void setup_node(db::node *node)
   {
      vm::predicate *init_pred(vm::theProgram->get_init_predicate());
      vm::tuple *init_tuple(vm::tuple::create(init_pred, &(node->alloc)));
      node->set_owner(this);
      node->add_linear_fact(init_tuple, init_pred);
      node->unprocessed_facts = true;
   }
   
public:

   db::node *thread_node{nullptr};

   // allows the core machine to stop threads
   static std::atomic<bool> stop_flag;

   inline vm::process_id get_id(void) const { return id; }
   inline utils::randgen *get_random() { return &rand; }
   inline utils::randgen *get_random() const { return &rand; }
   
   inline bool leader_thread(void) const { return get_id() == 0; }

   size_t queue_size() const {
      return queues.stati.size() + queues.moving.size() + prios.stati.size() + prios.moving.size();
   }

   db::node* init_node(db::database::map_nodes::iterator it)
   {
      db::node *node(vm::All->DATABASE->create_node_iterator(it));
      vm::theProgram->fix_node_address(node);
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

   inline void commit_nodes()
   {
      node_handler.commit();
   }

   void loop();
#ifndef COMPILED
	void run_const_code();
#endif
   void init(const size_t);
   
   void new_thread_work(thread *, vm::tuple *, const vm::predicate *);
   void new_work(db::node *, db::node *, vm::tuple *, vm::predicate *, const vm::derivation_direction, const vm::depth_t);
   void new_work_list(db::node *from, db::node *to, vm::buffer_node &b)  __attribute__((always_inline)) {
      assert(is_active());
      (void)from;

      LOCK_STACK(nodelock);

      NODE_LOCK(to, nodelock, node_lock);

      thread *owner(to->get_owner());

      if (owner == this) {
         {
            MUTEX_LOCK_GUARD(to->database_lock, database_lock);
            to->add_work_myself(b);
         }
#ifdef INSTRUMENTATION
         sent_facts_same_thread += b.size();
         all_transactions++;
#endif
         if (!to->active_node()) add_to_queue(to);
      } else {
         LOCK_STACK(databaselock);
         if (to->database_lock.try_lock1(LOCK_STACK_USE(databaselock))) {
            LOCKING_STAT(database_lock_ok);
            to->add_work_myself(b);
#ifdef INSTRUMENTATION
            sent_facts_other_thread_now += b.size();
            all_transactions++;
            thread_transactions++;
#endif
            MUTEX_UNLOCK(to->database_lock, databaselock);
         } else {
            LOCKING_STAT(database_lock_fail);
#ifdef INSTRUMENTATION
            sent_facts_other_thread += b.size();
            all_transactions++;
            thread_transactions++;
#endif
            to->add_work_others(b);
         }
         if (!to->active_node()) {
            owner->add_to_queue(to);
            comm_threads.set_bit(owner->get_id());
         }
      }

      NODE_UNLOCK(to, nodelock);
}

   void new_work_delay(db::node *, db::node *, vm::tuple*, vm::predicate *,
         const vm::derivation_direction, const vm::depth_t, const vm::uint_val)
   {
      assert(false);
   }
   
   db::node* get_work(void);
   void end(void);

   void set_node_cpu(db::node *, sched::thread*);
   void set_node_static(db::node *);
   void set_node_moving(db::node *);
   void set_node_affinity(db::node *, db::node *);
   void set_default_node_priority(db::node *, const vm::priority_t);
	void add_node_priority(db::node *, const vm::priority_t);
   void remove_node_priority(db::node *);
   void schedule_next(db::node *);
#include "thread/coord-include.hpp"

   inline uint64_t num_static_nodes(void) const { return static_nodes; }
#ifdef LOCK_STATISTICS
   void print_average_priority_size(void) const;
#endif

#ifdef INSTRUMENTATION
   void write_slice(statistics::slice&);
#endif

   static void init_barriers(const size_t);
   
   explicit thread(const vm::process_id);
   
   ~thread(void);
};

}

struct thread_hash {
   size_t operator()(const sched::thread *x) const {
      return std::hash<vm::process_id>()(x->get_id());
   }
};

#endif
