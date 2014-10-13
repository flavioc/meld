#include <iostream>
#include <climits>
#include <thread>

#include "thread/threads.hpp"
#include "db/database.hpp"
#include "process/remote.hpp"
#include "vm/state.hpp"
#include "sched/common.hpp"
#include "interface.hpp"

using namespace std;
using namespace process;
using namespace vm;
using namespace db;
using namespace utils;

//#define DEBUG_PRIORITIES
//#define PROFILE_QUEUE
#define SEND_OTHERS
#ifdef DIRECT_PRIORITIES
#undef SEND_OTHERS
#endif

// node stealing strategies.
//#define STEAL_ONE
#define STEAL_HALF

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

#ifdef INSTRUMENTATION
#define NODE_LOCK(NODE) do { if((NODE)->try_lock()) { node_lock_ok++; } else {node_lock_fail++; (NODE)->lock(); } while(false)
#else
#define NODE_LOCK(NODE) ((NODE)->lock())
#endif
#define NODE_UNLOCK(NODE) ((NODE)->unlock())

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
   assert_static_nodes_end(id);
}

void
threads_sched::new_work_list(db::node *from, db::node *to, vm::tuple_array& arr)
{
   assert(is_active());
   (void)from;

   thread_intrusive_node *tnode(dynamic_cast<thread_intrusive_node*>(to));

   NODE_LOCK(tnode);

   LOCK_STAT(add_lock);
   
   threads_sched *owner(dynamic_cast<threads_sched*>(tnode->get_owner()));

   if(owner == this) {
#ifdef FACT_STATISTICS
      count_add_work_self++;
#endif
      tnode->internal_lock();
      LOCK_STAT(internal_locks);
      tnode->add_work_myself(arr);
      tnode->internal_unlock();
#ifdef INSTRUMENTATION
      sent_facts_same_thread++;
#endif
      if(!tnode->active_node())
         add_to_queue(tnode);
   } else {
#ifdef FACT_STATISTICS
      count_add_work_other++;
#endif
      if(tnode->try_internal_lock()) {
         LOCK_STAT(internal_ok_locks);
         tnode->add_work_myself(arr);
#ifdef INSTRUMENTATION
         sent_facts_other_thread_now++;
#endif
         tnode->internal_unlock();
      } else {
         LOCK_STAT(internal_failed_locks);
         tnode->add_work_others(arr);
#ifdef INSTRUMENTATION
         sent_facts_other_thread++;
#endif
      }
      if(!tnode->active_node()) {
         owner->add_to_queue(tnode);
         owner->activate_thread();
      }
   }

   NODE_UNLOCK(tnode);
}

void
threads_sched::new_work(node *from, node *to, vm::tuple *tpl, vm::predicate *pred, const ref_count count, const depth_t depth)
{
   assert(is_active());
   (void)from;
   
   thread_intrusive_node *tnode(dynamic_cast<thread_intrusive_node*>(to));

   NODE_LOCK(tnode);

   LOCK_STAT(add_lock);
   
   threads_sched *owner(dynamic_cast<threads_sched*>(tnode->get_owner()));

   if(owner == this) {
#ifdef FACT_STATISTICS
      count_add_work_self++;
#endif
      tnode->internal_lock();
      LOCK_STAT(internal_locks);
      tnode->add_work_myself(tpl, pred, count, depth);
      tnode->internal_unlock();
#ifdef INSTRUMENTATION
      sent_facts_same_thread++;
#endif
      if(!tnode->active_node())
         add_to_queue(tnode);
   } else {
#ifdef FACT_STATISTICS
      count_add_work_other++;
#endif
      if(tnode->try_internal_lock()) {
         LOCK_STAT(internal_ok_locks);
         tnode->add_work_myself(tpl, pred, count, depth);
#ifdef INSTRUMENTATION
         sent_facts_other_thread_now++;
#endif
         tnode->internal_unlock();
      } else {
         LOCK_STAT(internal_failed_locks);
         tnode->add_work_others(tpl, pred, count, depth);
#ifdef INSTRUMENTATION
      sent_facts_other_thread++;
#endif
      }
      if(!tnode->active_node()) {
         owner->add_to_queue(tnode);
         owner->activate_thread();
      }
   }

   NODE_UNLOCK(tnode);
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
   thread_intrusive_node *node_buffer[NODE_BUFFER_SIZE];
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
         thread_intrusive_node *node(node_buffer[i]);

         NODE_LOCK(node);
         LOCK_STAT(steal_locks);
         if(node->node_state() != STATE_STEALING) {
            // node was put in the queue again, give up.
            NODE_UNLOCK(node);
            continue;
         }
         if(node->is_static()) {
            if(node->get_owner() != target) {
               // set-affinity was used and the node was changed to another scheduler
               move_node_to_new_owner(node, dynamic_cast<threads_sched*>(node->get_owner()));
               NODE_UNLOCK(node);
               continue;
            } else {
               // meanwhile the node is now set as static.
               // set ourselves as the static scheduler
               node->set_static(this);
            }
         }
         node->set_owner(this);
         add_to_queue(node);
         NODE_UNLOCK(node);
      }
      // set the next thread to the current one
      next_thread = tid;
      return true;
   }

   return false;
}

size_t
threads_sched::steal_nodes(thread_intrusive_node **buffer, const size_t max)
{
   steal_flag = !steal_flag;
   size_t stolen = 0;

#ifdef STEAL_ONE
   if(max == 0)
      return 0;

   thread_intrusive_node *node(NULL);
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
      else
         stolen = queues.moving.pop_tail_half(buffer, max, STATE_STEALING);
   }
#else
#error "Must select a way to steal nodes."
#endif
   return stolen;
}
#endif

void
threads_sched::generate_aggs(void)
{
   iterate_static_nodes(id);
}

void
threads_sched::killed_while_active(void)
{
   lock_guard<utils::mutex> l(lock);
   if(is_active())
      set_inactive();
}

#ifndef DIRECT_PRIORITIES
void
threads_sched::check_priority_buffer(void)
{
#ifdef SEND_OTHERS
   if(priority_buffer.size() == 0)
      return;

   size_t size(priority_buffer.pop(priority_tmp));

   for(size_t i(0); i < size; ++i) {
      priority_add_item p(priority_tmp[i]);
      node *target(p.target);
      thread_intrusive_node *tn((thread_intrusive_node *)target);
      if(tn == NULL)
         continue;
      const double howmuch(p.val);
      priority_add_type typ(p.typ);
      if(tn->get_owner() != this)
         continue;
#ifdef TASK_STEALING
      NODE_LOCK(tn);
		if(tn->get_owner() == this) {
         switch(typ) {
            case ADD_PRIORITY:
               do_set_node_priority(tn, tn->get_priority_level() + howmuch);
               break;
            case SET_PRIORITY:
               do_set_node_priority(tn, howmuch);
               break;
         }
      }
      NODE_UNLOCK(tn);
#endif
   }
#endif
}
#endif

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
#define STEALING_ROUND_MAX 10
#define BACKOFF_INCREASE_FACTOR 4
#define BACKOFF_DECREASE_FACTOR 2
      if(!theProgram->is_static_priority() && work_stealing) {
         count++;
         if(count == backoff) {
            count = 0;
            set_active_if_inactive();
            if(go_steal_nodes()) {
               ins_active;
               backoff = max(backoff / BACKOFF_DECREASE_FACTOR, (size_t)STEALING_ROUND_MAX);
               return true;
            } else {
               if(backoff < UINT_MAX/BACKOFF_INCREASE_FACTOR)
                  backoff *= BACKOFF_INCREASE_FACTOR;
            }
         }
      }
#endif
      if(is_active() && !has_work()) {
         std::lock_guard<utils::mutex> l(lock);
         if(!has_work() && is_active())
            set_inactive();
      } else
         break;
      ins_idle;
      if(all_threads_finished() || stop_flag) {
         assert(is_inactive());
         return false;
      }
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
      NODE_LOCK(current_node);
      LOCK_STAT(check_lock);
      
      if(!current_node->unprocessed_facts) {
         current_node->make_inactive();
         if(current_node->is_static() && current_node->get_static() != this) {
            // the node has changed to another scheduler!
            current_node->set_owner(current_node->get_static());
         }
         current_node->set_priority_level(0.0);
         NODE_UNLOCK(current_node);
         current_node = NULL;
         return true;
      }
      NODE_UNLOCK(current_node);
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
            //     cout << "Got node " << current_node->get_id() << " with prio " << current_node->get_priority_level() << endl;
            break;
         }
      }

      if(pop_node_from_queues()) {
  //     cout << "Got node " << current_node->get_id() << endl;
         break;
      }
      if(!has_work()) {
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
	
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      thread_intrusive_node *cur_node((thread_intrusive_node*)it->second);
		
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
threads_sched::schedule_next(node *n)
{
   static const double add = 100.0;
   double prio(0.0);

   if(!prios.moving.empty())
      prio = prios.moving.min_value();
   if(!prios.stati.empty())
      prio = max(prio, prios.stati.min_value());

   prio += add;

#ifdef TASK_STEALING
   thread_intrusive_node *tn((thread_intrusive_node*)n);
   NODE_LOCK(tn);
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this)
      do_set_node_priority(n, prio);

   NODE_UNLOCK(tn);
#else
   do_set_node_priority(n, prio);
#endif
}

void
threads_sched::add_node_priority_other(node *n, const double priority)
{
#ifdef SEND_OTHERS
   thread_intrusive_node *tn((thread_intrusive_node*)n);
   threads_sched *other((threads_sched*)tn->get_owner());

   priority_add_item item;
   item.typ = ADD_PRIORITY;
   item.val = priority;
   item.target = n;

   other->priority_buffer.add(item);
#else
   (void)n;
   (void)priority;
#endif
}

static inline bool
is_higher_priority(thread_intrusive_node *tn, const double priority)
{
   if(theProgram->is_priority_desc())
      return tn->get_priority_level() < priority;
   else
      return tn->get_priority_level() > priority;
}

void
threads_sched::set_node_priority_other(node *n, const double priority)
{
#ifdef SEND_OTHERS
   thread_intrusive_node *tn((thread_intrusive_node*)n);

   if(!is_higher_priority(tn, priority))
      // does not change
      return;

   // will potentially change
   threads_sched *other((threads_sched*)tn->get_owner());
   priority_add_item item;
   item.typ = SET_PRIORITY;
   item.val = priority;

   other->priority_buffer.add(item);
#else
   (void)priority;
   (void)n;
#endif
}

void
threads_sched::add_node_priority(node *n, const double priority)
{
   if(!scheduling_mechanism)
      return;
#ifdef FACT_STATISTICS
   count_add_priority++;
#endif

	thread_intrusive_node *tn((thread_intrusive_node*)n);

#ifdef TASK_STEALING
   NODE_LOCK(tn);
   LOCK_STAT(prio_lock);
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this)
      do_set_node_priority(n, tn->get_priority_level() + priority);
   else
#ifdef DIRECT_PRIORITIES
      do_set_node_priority_other(tn, tn->get_priority_level() + priority);
#else
      other->add_node_priority_other(tn, priority);
#endif
   NODE_UNLOCK(tn);
#else
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this) {
      const double old_prio(tn->get_float_priority_level());
      set_node_priority(n, old_prio + priority);
   }
#endif
}

void
threads_sched::set_default_node_priority(node *n, const double priority)
{
   if(!scheduling_mechanism)
      return;

   thread_intrusive_node *tn((thread_intrusive_node*)n);
//   cout << "Default priority " << priority << endl;
   tn->set_default_priority_level(priority);
}

void
threads_sched::set_node_priority(node *n, const double priority)
{
   if(!scheduling_mechanism)
      return;

   thread_intrusive_node *tn((thread_intrusive_node*)n);
#ifdef FACT_STATISTICS
   count_set_priority++;
#endif

//   cout << "Set node " << n->get_id() << " with prio " << priority << endl;
#ifdef TASK_STEALING
   NODE_LOCK(tn);
   LOCK_STAT(prio_lock);
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this)
      do_set_node_priority(n, priority);
   else
#ifdef DIRECT_PRIORITIES
      set_node_priority_other(n, priority);
#else
      other->set_node_priority_other(n, priority);
#endif
   NODE_UNLOCK(tn);
#else
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this)
      do_set_node_priority(n, priority);
#endif
}

void
threads_sched::do_set_node_priority_other(thread_intrusive_node *node, const double priority)
{
#ifdef INSTRUMENTATION
   priority_nodes_others++;
#endif
   // we know that the owner of node is not this.
   queue_id_t state(node->node_state());
   threads_sched *owner(dynamic_cast<threads_sched*>(node->get_owner()));
   bool activate_node(false);

   switch(state) {
      case STATE_STEALING:
         // node is being stolen.
         if(is_higher_priority(node, priority))
            node->set_priority_level(priority);
         return;
      case NORMAL_QUEUE_MOVING:
         if(owner->queues.moving.remove(node, STATE_PRIO_CHANGE)) {
            owner->add_to_priority_queue(node);
            activate_node = true;
         }
         break;
      case NORMAL_QUEUE_STATIC:
         if(owner->queues.stati.remove(node, STATE_PRIO_CHANGE)) {
            owner->add_to_priority_queue(node);
            activate_node = true;
         }
         break;
      case PRIORITY_MOVING:
         if(priority == 0) {
            node->set_priority_level(0.0);
            if(owner->prios.moving.remove(node, STATE_PRIO_CHANGE)) {
               owner->queues.moving.push_tail(node);
               activate_node = true;
            }
         } else if(is_higher_priority(node, priority)) {
            node->set_priority_level(priority);
            owner->prios.moving.move_node(node, priority);
         }
         break;
      case PRIORITY_STATIC:
         if(priority == 0) {
            node->set_priority_level(0.0);
            if(owner->prios.stati.remove(node, STATE_PRIO_CHANGE)) {
               owner->queues.stati.push_tail(node);
               activate_node = true;
            }
         } else if(is_higher_priority(node, priority)) {
            node->set_priority_level(priority);
            owner->prios.stati.move_node(node, priority);
         }
         break;
      case STATE_WORKING:
         // do nothing
         break;
      default: assert(false); break;
   } 
   if(activate_node) {
      // we must reactivate node if it's not active
      lock_guard<utils::mutex> l2(owner->lock);
      
      if(owner->is_inactive()) {
         owner->set_active();
         assert(owner->is_active());
      }
   }
}

void
threads_sched::do_set_node_priority(node *n, const double priority)
{
	thread_intrusive_node *tn((thread_intrusive_node*)n);

#ifdef DEBUG_PRIORITIES
   if(priority > 0)
      tn->has_been_prioritized = true;
#endif
   if(priority < 0)
      return;
#ifdef INSTRUMENTATION
   priority_nodes_thread++;
#endif

   if(current_node == tn) {
      tn->set_priority_level(priority);
      return;
   }

   switch(tn->node_state()) {
      case STATE_STEALING:
         // node is being stolen, let's simply set the level
         // and let the other thread do it's job
         if(is_higher_priority(tn, priority))
            // we check if new priority is bigger than the current priority
            tn->set_priority_level(priority);
         break;
      case PRIORITY_MOVING:
         if(priority == 0.0) {
            tn->set_priority_level(0.0);
            if(prios.moving.remove(tn, STATE_PRIO_CHANGE))
               add_to_queue(tn);
         } else if(is_higher_priority(tn, priority)) {
            // we check if new priority is bigger than the current priority
            tn->set_priority_level(priority);
            prios.moving.move_node(tn, priority);
         }
         break;
      case PRIORITY_STATIC:
         if(priority == 0.0) {
            tn->set_priority_level(0.0);
            if(prios.stati.remove(tn, STATE_PRIO_CHANGE))
               add_to_queue(tn);
         } else if(is_higher_priority(tn, priority)) {
            // we check if new priority is bigger than the current priority
            tn->set_priority_level(priority);
            prios.stati.move_node(tn, priority);
         }
         break;
      case NORMAL_QUEUE_MOVING:
         tn->set_priority_level(priority);
         if(queues.moving.remove(tn, STATE_PRIO_CHANGE))
            add_to_priority_queue(tn);
         break;
      case NORMAL_QUEUE_STATIC:
         tn->set_priority_level(priority);
         if(queues.stati.remove(tn, STATE_PRIO_CHANGE))
            add_to_priority_queue(tn);
         break;
      case STATE_WORKING:
         if(is_higher_priority(tn, priority))
            tn->set_priority_level(priority);
         break;
      case STATE_IDLE:
         tn->set_priority_level(priority);
         break;
      default: assert(false); break;
   }
#ifdef PROFILE_QUEUE
   prio_marked++;
	prio_count++;
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

   database::map_nodes::iterator it(All->DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(All->DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   const double initial(theProgram->get_initial_priority());

   if(initial == 0.0 || !scheduling_mechanism) {
      for(; it != end; ++it)
      {
         thread_intrusive_node *cur_node(dynamic_cast<thread_intrusive_node*>(init_node(it)));
      	queues.moving.push_tail(cur_node);
      }
   } else {
      prios.moving.start_initial_insert(remote::self->find_owned_nodes(id));

      for(size_t i(0); it != end; ++it, ++i) {
         thread_intrusive_node *cur_node(dynamic_cast<thread_intrusive_node*>(init_node(it)));

         cur_node->set_priority_level(initial);
         prios.moving.initial_fast_insert(cur_node, initial, i);
      }
   }
   
   threads_synchronize();
}

void
threads_sched::set_node_static(db::node *n)
{
   if(!scheduling_mechanism)
      return;

   //cout << "Static " << n->get_id() << endl;

   thread_intrusive_node *tn(dynamic_cast<thread_intrusive_node*>(n));
   if(n == current_node) {
      tn->set_static(this);
      return;
   }

   if(tn->is_static())
      return;

   NODE_LOCK(tn);
   tn->set_static(this);
   switch(tn->node_state()) {
      case STATE_STEALING:
         // node was stolen but the thief does not
         // know yet that this node is now static!
         // since the static field was set, the
         // thief knows that he needs to do something.
         break;
      case NORMAL_QUEUE_MOVING:
         if(queues.moving.remove(tn, STATE_STATIC_CHANGE))
            queues.stati.push_tail(tn);
         break;
      case PRIORITY_MOVING:
         if(prios.moving.remove(tn, STATE_STATIC_CHANGE))
            prios.stati.insert(tn, tn->get_priority_level());
         break;
      case NORMAL_QUEUE_STATIC:
      case PRIORITY_STATIC:
      case STATE_IDLE:
      case STATE_WORKING:
         // do nothing
         break;
      default: assert(false); break;
   }
   NODE_UNLOCK(tn);
}

void
threads_sched::set_node_moving(db::node *n)
{
   if(!scheduling_mechanism)
      return;

//   cout << "Moving " << n->get_id() << endl;

   thread_intrusive_node *tn(dynamic_cast<thread_intrusive_node*>(n));
   if(n == current_node) {
      tn->set_moving();
      return;
   }

   if(tn->is_moving())
      return;

   NODE_LOCK(tn);
   tn->set_moving();
   switch(tn->node_state()) {
      case STATE_STEALING:
         // node was stolen but the thief does not
         // know yet that this node is now static!
         // since the static field was set, the
         // thief knows that he needs to do something.
         break;
      case NORMAL_QUEUE_STATIC:
         if(queues.stati.remove(tn, STATE_STATIC_CHANGE))
            queues.moving.push_tail(tn);
         break;
      case PRIORITY_STATIC:
         if(prios.stati.remove(tn, STATE_STATIC_CHANGE))
            prios.moving.insert(tn, tn->get_priority_level());
         break;
      case NORMAL_QUEUE_MOVING:
      case PRIORITY_MOVING:
      case STATE_WORKING:
      case STATE_IDLE:
         // do nothing
         break;
      default: assert(false); break;
   }
   NODE_UNLOCK(tn);
}

void
threads_sched::move_node_to_new_owner(thread_intrusive_node *tn, threads_sched *new_owner)
{
   new_owner->add_to_queue(tn);

   lock_guard<utils::mutex> l2(new_owner->lock);
   
   if(new_owner->is_inactive())
   {
      new_owner->set_active();
      assert(new_owner->is_active());
   }
}

void
threads_sched::set_node_affinity(db::node *node, db::node *affinity)
{
   threads_sched *new_owner(dynamic_cast<threads_sched*>(affinity->get_owner()));
   thread_intrusive_node *tn(dynamic_cast<thread_intrusive_node*>(node));

   if(tn == current_node) {
      // we will change the owner field once we are done with the node.
      tn->set_static(new_owner);
      return;
   }

   NODE_LOCK(tn);
   tn->set_static(new_owner);
   if(tn->get_owner() == new_owner) {
      NODE_UNLOCK(tn);
      return;
   }

   switch(tn->node_state()) {
      case STATE_STEALING:
         // node is being stolen right now!
         // change both owner and is_static
         tn->set_owner(new_owner);
         break;
      case NORMAL_QUEUE_STATIC:
         if(queues.stati.remove(tn, STATE_AFFINITY_CHANGE)) {
            tn->set_owner(new_owner);
            move_node_to_new_owner(tn, new_owner);
         }
         break;
      case NORMAL_QUEUE_MOVING:
         if(queues.moving.remove(tn, STATE_AFFINITY_CHANGE)) {
            tn->set_owner(new_owner);
            move_node_to_new_owner(tn, new_owner);
         }
         break;
      case PRIORITY_MOVING:
         if(prios.moving.remove(tn, STATE_AFFINITY_CHANGE)) {
            tn->set_owner(new_owner);
            move_node_to_new_owner(tn, new_owner);
         }
         break;
      case PRIORITY_STATIC:
         if(prios.stati.remove(tn, STATE_AFFINITY_CHANGE)) {
            tn->set_owner(new_owner);
            move_node_to_new_owner(tn, new_owner);
         }
         break;
      case STATE_WORKING:
         // we set static to the new owner
         // the scheduler using that node right now will
         // change its owner once it finishes processing the node
         break;
      case STATE_IDLE:
         tn->set_owner(new_owner);
         break;
      default: assert(false); break; 
   }

   NODE_UNLOCK(tn);
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
   , rand(time(NULL) + _id * 10)
   , next_thread(rand(All->NUM_THREADS))
   , backoff(STEALING_ROUND_MAX)
#endif
#ifndef DIRECT_PRIORITIES
   , priority_buffer(std::min(PRIORITY_BUFFER_SIZE,
                     vm::All->DATABASE->num_nodes() / vm::All->NUM_THREADS))
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
{
}

threads_sched::~threads_sched(void)
{
   assert(tstate == THREAD_INACTIVE);
}
   
}

