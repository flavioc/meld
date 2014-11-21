#include <iostream>
#include <climits>
#include <thread>

#include "thread/threads.hpp"
#include "db/database.hpp"
#include "vm/state.hpp"
#include "interface.hpp"
#include "vm/priority.hpp"
#include "machine.hpp"

using namespace std;
using namespace process;
using namespace vm;
using namespace db;
using namespace utils;

#ifdef PROFILE_QUEUE
static atomic<size_t> prio_count(0);
static atomic<size_t> prio_immediate(0);
static atomic<size_t> prio_marked(0);
static atomic<size_t> prio_moved_pqueue(0);
static atomic<size_t> prio_add_pqueue(0);
static atomic<size_t> prio_removed_pqueue(0);
static atomic<size_t> prio_nodes_compared(0);
static atomic<size_t> prio_nodes_changed(0);
#endif

namespace sched
{

tree_barrier* threads_sched::thread_barrier(NULL);
termination_barrier* threads_sched::term_barrier(NULL);
#ifdef TASK_STEALING
std::atomic<int> *threads_sched::steal_states(NULL);
#endif

void
threads_sched::init_barriers(const size_t num_threads)
{
   thread_barrier = new tree_barrier(num_threads);
   term_barrier = new termination_barrier(num_threads);
#ifdef TASK_STEALING
   steal_states = new std::atomic<int>[num_threads];
   for(size_t i(0); i < num_threads; ++i)
      steal_states[i] = 1;
#endif
}
   
void
threads_sched::assert_end(void) const
{
   assert(is_inactive());
   assert(all_threads_finished());
}

void
threads_sched::new_work_list(db::node *from, db::node *to, vm::tuple_array& arr)
{
   assert(is_active());
   (void)from;

   LOCK_STACK(nodelock);

   NODE_LOCK(to, nodelock, node_lock);
   
   threads_sched *owner(static_cast<threads_sched*>(to->get_owner()));

   if(owner == this) {
#ifdef FACT_STATISTICS
      count_add_work_self++;
#endif
      {
         MUTEX_LOCK_GUARD(to->database_lock, database_lock);
         to->add_work_myself(arr);
      }
#ifdef INSTRUMENTATION
      sent_facts_same_thread++;
#endif
      if(!to->active_node())
         add_to_queue(to);
   } else {
#ifdef FACT_STATISTICS
      count_add_work_other++;
#endif
      LOCK_STACK(databaselock);
      if(to->database_lock.try_lock1(LOCK_STACK_USE(databaselock))) {
         LOCKING_STAT(database_lock_ok);
         to->add_work_myself(arr);
#ifdef INSTRUMENTATION
         sent_facts_other_thread_now++;
#endif
         MUTEX_UNLOCK(to->database_lock, databaselock);
      } else {
         LOCKING_STAT(database_lock_fail);
         to->add_work_others(arr);
#ifdef INSTRUMENTATION
         sent_facts_other_thread++;
#endif
      }
      if(!to->active_node()) {
         owner->add_to_queue(to);
         comm_threads.set_bit(owner->get_id());
      }
   }

   NODE_UNLOCK(to, nodelock);
}

void
threads_sched::new_work(node *from, node *to, vm::tuple *tpl, vm::predicate *pred, const ref_count count, const depth_t depth)
{
   assert(is_active());
   (void)from;
   
   LOCK_STACK(nodelock);
   NODE_LOCK(to, nodelock, node_lock);
   
   threads_sched *owner(static_cast<threads_sched*>(to->get_owner()));
   if(owner == this) {
#ifdef FACT_STATISTICS
      count_add_work_self++;
#endif
      {
         MUTEX_LOCK_GUARD(to->database_lock, database_lock);
         to->add_work_myself(tpl, pred, count, depth);
      }
#ifdef INSTRUMENTATION
      sent_facts_same_thread++;
#endif
      if(!to->active_node())
         add_to_queue(to);
   } else {
#ifdef FACT_STATISTICS
      count_add_work_other++;
#endif
      LOCK_STACK(databaselock);

      if(to->database_lock.try_lock1(LOCK_STACK_USE(databaselock))) {
         LOCKING_STAT(database_lock_ok);
         to->add_work_myself(tpl, pred, count, depth);
#ifdef INSTRUMENTATION
         sent_facts_other_thread_now++;
#endif
         MUTEX_UNLOCK(to->database_lock, databaselock);
      } else {
         LOCKING_STAT(database_lock_fail);
         to->add_work_others(tpl, pred, count, depth);
#ifdef INSTRUMENTATION
      sent_facts_other_thread++;
#endif
      }
      if(!to->active_node()) {
         owner->add_to_queue(to);
         comm_threads.set_bit(owner->get_id());
      }
   }

   NODE_UNLOCK(to, nodelock);
}

#ifdef TASK_STEALING
bool
threads_sched::go_steal_nodes(void)
{
   if(All->NUM_THREADS == 1)
      return false;

   ins_sched;
   assert(is_active());
#ifdef STEAL_ONE
#define NODE_BUFFER_SIZE 1
#elif defined(STEAL_HALF)
#define NODE_BUFFER_SIZE 10
#endif
   db::node *node_buffer[NODE_BUFFER_SIZE];
   int state_buffer[All->NUM_THREADS];

   memcpy(state_buffer, steal_states, sizeof(int) * All->NUM_THREADS);

   for(size_t i(0); i < All->NUM_THREADS; ++i) {
      size_t tid((next_thread + i) % All->NUM_THREADS);
      if(tid == get_id())
         continue;

      if(state_buffer[tid] == 0)
         continue;

      threads_sched *target((threads_sched*)All->SCHEDS[tid]);

      if(!target->is_active() || !target->has_work())
         continue;
      size_t stolen(target->steal_nodes(node_buffer, NODE_BUFFER_SIZE));

      if(stolen == 0)
         continue;
#ifdef FACT_STATISTICS
      count_stolen_nodes += stolen;
#endif
#ifdef INSTRUMENTATION
      stolen_total += stolen;
#endif

      for(size_t i(0); i < stolen; ++i) {
         db::node *node(node_buffer[i]);

         LOCK_STACK(nodelock);
         NODE_LOCK(node, nodelock, node_lock);
         if(node->node_state() != STATE_STEALING) {
            // node was put in the queue again, give up.
            NODE_UNLOCK(node, nodelock);
            continue;
         }
         if(node->is_static()) {
            if(node->get_owner() != target) {
               // set-affinity was used and the node was changed to another scheduler
               move_node_to_new_owner(node, static_cast<threads_sched*>(node->get_owner()));
               NODE_UNLOCK(node, nodelock);
               continue;
            } else {
               // meanwhile the node is now set as static.
               // set ourselves as the static scheduler
               make_node_static(node, this);
            }
         }
         node->set_owner(this);
         add_to_queue(node);
         NODE_UNLOCK(node, nodelock);
      }
      // set the next thread to the current one
      next_thread = tid;
      return true;
   }

   return false;
}

size_t
threads_sched::steal_nodes(db::node **buffer, const size_t max)
{
   steal_flag = !steal_flag;
   size_t stolen = 0;

#ifdef STEAL_ONE
   if(max == 0)
      return 0;

   db::node *node(NULL);
   if(steal_flag) {
      if(!queues.moving.empty()) {
         if(queues.moving.pop_tail(node, STATE_STEALING)) {
            buffer[stolen++] = node;
         }
      } else if(!prios.moving.empty()) {
         node = prios.moving.pop(STATE_STEALING);
         if(node)
            buffer[stolen++] = node;
      }
   } else {
      if(!prios.moving.empty()) {
         node = prios.moving.pop(STATE_STEALING);
         if(node)
            buffer[stolen++] = node;
      } else if(!queues.moving.empty()) {
         if(queues.moving.pop_head(node, STATE_STEALING))
            buffer[stolen++] = node;
      }
   }
#elif defined(STEAL_HALF)
   if(steal_flag) {
      if(!queues.moving.empty())
         stolen = queues.moving.pop_tail_half(buffer, max, STATE_STEALING);
      else if(!prios.moving.empty())
         stolen = prios.moving.pop_half(buffer, max, STATE_STEALING);
   } else {
      if(!prios.moving.empty())
         stolen = prios.moving.pop_half(buffer, max, STATE_STEALING);
      else if(!queues.moving.empty())
         stolen = queues.moving.pop_tail_half(buffer, max, STATE_STEALING);
   }
#else
#error "Must select a way to steal nodes."
#endif
   return stolen;
}
#endif

void
threads_sched::killed_while_active(void)
{
   MUTEX_LOCK_GUARD(lock, thread_lock);
   if(is_active())
      set_inactive();
}

bool
threads_sched::busy_wait(void)
{
#ifdef TASK_STEALING
   if(!theProgram->is_static_priority()) {
      if(work_stealing && go_steal_nodes()) {
         ins_active;
         return true;
      }
   }
#endif

#ifdef TASK_STEALING
   size_t count(0);
#endif
   
   while(!has_work()) {
#ifdef TASK_STEALING
#define STEALING_ROUND_MIN 16
#define STEALING_ROUND_MAX 4096
      if(!theProgram->is_static_priority() && work_stealing) {
         count++;
         if(count == backoff) {
            count = 0;
            set_active_if_inactive();
            if(go_steal_nodes()) {
               ins_active;
               backoff = max(backoff >> 1, (size_t)STEALING_ROUND_MIN);
               return true;
            } else
               backoff = min((size_t)STEALING_ROUND_MAX, backoff << 1);
         }
      }
#endif
      if(is_active() && !has_work()) {
         MUTEX_LOCK_GUARD(lock, thread_lock);
         if(!has_work()) {
            if(is_active())
               set_inactive();
         } else break;
      }
      ins_idle;
      if(all_threads_finished() || stop_flag) {
         assert(is_inactive());
         return false;
      }
      cpu_relax();
      std::this_thread::yield();
   }
   
   // since queue pushing and state setting are done in
   // different exclusive regions, this may be needed
   set_active_if_inactive();
   ins_active;
   assert(is_active());
   
   return true;
}

bool
threads_sched::check_if_current_useless(void)
{
   if(!current_node->unprocessed_facts) {
      LOCK_STACK(curlock);
      NODE_LOCK(current_node, curlock, node_lock);
      
      if(!current_node->unprocessed_facts) {
         current_node->make_inactive();
         if(current_node->is_static() && current_node->get_static() != this) {
            // the node has changed to another scheduler!
            current_node->set_owner(current_node->get_static());
         }
         current_node->remove_temporary_priority();
         NODE_UNLOCK(current_node, curlock);
#ifdef GC_NODES
         if(current_node->garbage_collect())
            delete_node(current_node);
#endif
         current_node = NULL;
         return true;
      }
      NODE_UNLOCK(current_node, curlock);
   }
   
   assert(current_node->unprocessed_facts);
   return false;
}

bool
threads_sched::set_next_node(void)
{
#ifndef DIRECT_PRIORITIES
   check_priority_buffer();
#endif

   if(current_node != NULL)
      check_if_current_useless();
   
   while (current_node == NULL) {   
      if(scheduling_mechanism) {
         current_node = prios.moving.pop_best(prios.stati, STATE_WORKING);
         if(current_node) {
            //cout << "Got node " << current_node->get_id() << " with prio " << current_node->get_priority() << endl;
            break;
         }
      }

      if(pop_node_from_queues()) {
         //cout << "Got node " << current_node->get_id() << endl;
         break;
      }
      if(!has_work()) {
         for(auto it(comm_threads.begin(All->NUM_THREADS)); !it.end(); ++it) {
            const size_t id(*it);
            threads_sched *target(static_cast<threads_sched*>(All->SCHEDS[id]));
            target->activate_thread();
         }
         comm_threads.clear(All->NUM_THREADS_NEXT_UINT);
         if(!busy_wait())
            return false;
      }
   }
   
   ins_active;
   
   assert(current_node != NULL);
   
   return true;
}

node*
threads_sched::get_work(void)
{  
   if(!set_next_node())
      return NULL;

   set_active_if_inactive();
   ins_active;
   assert(current_node != NULL);
   assert(current_node->unprocessed_facts);
   
   return current_node;
}

void
threads_sched::end(void)
{
#if defined(DEBUG_PRIORITIES) && defined(PROFILE_QUEUE)
	cout << "prio_immediate: " << prio_immediate << endl;
	cout << "prio_marked: " << prio_marked << endl;
	cout << "prio_count: " << prio_count << endl;
#endif
	
#if defined(DEBUG_PRIORITIES)
	size_t total_prioritized(0);
	size_t total_nonprioritized(0);
	
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(All->MACHINE->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(All->MACHINE->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      db::node *cur_node(it->second);
		
		if(cur_node->has_been_prioritized)
			++total_prioritized;
		else
			++total_nonprioritized;
	}
	
	cout << "Number of prioritized nodes: " << total_prioritized << endl;
	cout << "Number of non prioritized nodes: " << total_nonprioritized << endl;
#endif
}

void
threads_sched::init(const size_t)
{
   // normal priorities
   if(theProgram->is_priority_desc()) {
      prios.moving.set_type(HEAP_DESC);
      prios.stati.set_type(HEAP_DESC);
   } else {
      prios.moving.set_type(HEAP_ASC);
      prios.stati.set_type(HEAP_ASC);
   }

   database::map_nodes::iterator it(All->DATABASE->get_node_iterator(All->MACHINE->find_first_node(id)));
   database::map_nodes::iterator end(All->DATABASE->get_node_iterator(All->MACHINE->find_last_node(id)));
   const priority_t initial(theProgram->get_initial_priority());

   if(initial == no_priority_value() || !scheduling_mechanism) {
      for(; it != end; ++it)
      {
         db::node *cur_node(init_node(it));
      	queues.moving.push_tail(cur_node);
      }
   } else {
      prios.moving.start_initial_insert(All->MACHINE->find_owned_nodes(id));

      for(size_t i(0); it != end; ++it, ++i) {
         db::node *cur_node(init_node(it));

         prios.moving.initial_fast_insert(cur_node, initial, i);
      }
   }
   
   threads_synchronize();
}

void
threads_sched::write_slice(statistics::slice& sl)
{
#ifdef INSTRUMENTATION
   base::write_slice(sl);
   sl.work_queue = queues.stati.size() + queues.moving.size() + prios.stati.size() + prios.moving.size();
   sl.sent_facts_same_thread = sent_facts_same_thread;
   sl.sent_facts_other_thread = sent_facts_other_thread;
   sl.sent_facts_other_thread_now = sent_facts_other_thread_now;
   sl.priority_nodes_thread = priority_nodes_thread;
   sl.priority_nodes_others = priority_nodes_others;
   sl.bytes_used = All->THREAD_POOLS[get_id()]->bytes_in_use;
   sl.node_lock_ok = node_lock_ok;
   sl.node_lock_fail = node_lock_fail;
   node_lock_fail = 0;
   node_lock_ok = 0;
   sent_facts_same_thread = 0;
   sent_facts_other_thread = 0;
   sent_facts_other_thread_now = 0;
   priority_nodes_thread = 0;
   priority_nodes_others = 0;
#ifdef TASK_STEALING
   sl.stolen_nodes = stolen_total;
   stolen_total = 0;
#endif
#else
   (void)sl;
#endif
}

threads_sched::threads_sched(const vm::process_id _id):
   base(_id),
   tstate(THREAD_ACTIVE),
   current_node(NULL)
#ifdef TASK_STEALING
   , rand(_id * 1000)
   , next_thread(rand(All->NUM_THREADS))
   , backoff(STEALING_ROUND_MIN)
#endif
#ifdef INSTRUMENTATION
   , sent_facts_same_thread(0)
   , sent_facts_other_thread(0)
   , sent_facts_other_thread_now(0)
   , priority_nodes_thread(0)
   , priority_nodes_others(0)
   , node_lock_fail(0)
   , node_lock_ok(0)
#endif
#ifndef DIRECT_PRIORITIES
   , priority_buffer(std::min(PRIORITY_BUFFER_SIZE,
                     vm::All->DATABASE->num_nodes() / vm::All->NUM_THREADS))
#endif
   , static_nodes(0)
{
   bitmap::create(comm_threads, All->NUM_THREADS_NEXT_UINT);
   comm_threads.clear(All->NUM_THREADS_NEXT_UINT);
}

threads_sched::~threads_sched(void)
{
   bitmap::destroy(comm_threads, All->NUM_THREADS_NEXT_UINT);
   assert(tstate == THREAD_INACTIVE);
}
   
}

