#include <iostream>
#include <climits>

#include "thread/threads.hpp"
#include "db/database.hpp"
#include "db/tuple.hpp"
#include "process/remote.hpp"
#include "sched/thread/assert.hpp"
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

void
threads_sched::assert_end(void) const
{
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   assert_static_nodes_end(id);
}

void
threads_sched::assert_end_iteration(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   assert_static_nodes_end_iteration(id);
}

void
threads_sched::new_agg(work&)
{
   assert(false);
}

void
threads_sched::new_work(node *from, node *to, vm::tuple *tpl, vm::predicate *pred, const ref_count count, const depth_t depth)
{
   assert(is_active());
   (void)from;
   
   thread_intrusive_node *tnode(dynamic_cast<thread_intrusive_node*>(to));

   tnode->lock();
   
   threads_sched *owner(dynamic_cast<threads_sched*>(tnode->get_owner()));

   if(owner == this) {
      assert(!tnode->running);
      tnode->internal_lock();
      tnode->add_work_myself(tpl, pred, count, depth);
      tnode->internal_unlock();
#ifdef INSTRUMENTATION
      sent_facts_same_thread++;
#endif
      if(!tnode->active_node())
         add_to_queue(tnode);
   } else {
      if(tnode->running) {
         // the node is currently being executed by the owner thread
         // just buffer the new fact that will be used by the owner
         tnode->add_work_others(tpl, pred, count, depth);
      } else {
         // the node is asleep, we can add it immediatelly to the index
         tnode->internal_lock();
         tnode->add_work_myself(tpl, pred, count, depth);
         tnode->internal_unlock();
#ifdef INSTRUMENTATION
         sent_facts_other_thread_now++;
#endif
      }
      if(!tnode->active_node())
         owner->add_to_queue(tnode);
#ifdef INSTRUMENTATION
      sent_facts_other_thread++;
#endif

      lock_guard<mutex> l2(owner->lock);
      
      if(owner->is_inactive())
      {
         owner->set_active();
         assert(owner->is_active());
      }
   }

   tnode->unlock();
}

#ifdef COMPILE_MPI
void
threads_sched::new_work_remote(remote *, const node::node_id, message *)
{
   assert(false);
}
#endif

#ifdef TASK_STEALING
bool
threads_sched::go_steal_nodes(void)
{
   if(All->NUM_THREADS == 1)
      return false;

   ins_sched;
   assert(is_active());

   // roubar em ciclo
   // verificar se o rand se inicializa com seeds diferentes
   // tentar roubar mesmo que a fila so tenha 1 no

   for(size_t i(0); i < All->NUM_THREADS; ++i) {
      size_t tid((next_thread + i) % All->NUM_THREADS);
      if(tid == get_id())
         continue;

      threads_sched *target((threads_sched*)All->SCHEDS[tid]);

      if(!target->is_active() || !target->has_work())
         continue;
      size_t size = 1;

      assert(size > 0);

      while(size > 0) {
         thread_intrusive_node *node(target->steal_node());

         if(node == NULL)
            break;

         node->lock();
         if(node->node_state() != STATE_STEALING) {
            // node was put in the queue again, give up.
            node->unlock();
            continue;
         }
         if(node->is_static()) {
            if(node->get_owner() != target) {
               // set-affinity was used and the node was changed to another scheduler
               move_node_to_new_owner(node, dynamic_cast<threads_sched*>(node->get_owner()));
               node->unlock();
               continue;
            } else {
               // meanwhile the node is now set as static.
               // set ourselves as the static scheduler
               node->set_static(this);
            }
         }
         node->set_owner(this);
         add_to_queue(node);
         node->unlock();
#ifdef INSTRUMENTATION
         stolen_total++;
#endif
         --size;
      }
      if(size == 0) {
         // set the next thread to the current one
         next_thread = tid;
         return true;
      }
   }

   return false;
}

thread_intrusive_node*
threads_sched::steal_node(void)
{
   thread_intrusive_node *node(NULL);
   steal_flag = !steal_flag;

   if(steal_flag) {
      if(!queues.moving.empty())
         queues.moving.pop_tail(node, STATE_STEALING);
      else if(!prios.moving.empty())
         node = prios.moving.pop(STATE_STEALING);
   } else {
      if(!prios.moving.empty())
         node = prios.moving.pop(STATE_STEALING);
      else if(!queues.moving.empty())
         queues.moving.pop_head(node, STATE_STEALING);
   }
   return node;
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
   lock_guard<mutex> l(lock);
   if(is_active())
      set_inactive();
}

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
      tn->lock();
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
      tn->unlock();
#endif
   }
#endif
}

bool
threads_sched::busy_wait(void)
{
#ifdef TASK_STEALING
   if(!theProgram->is_static_priority() && go_steal_nodes()) {
      ins_active;
      return true;
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
      if(!theProgram->is_static_priority()) {
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
      ins_idle;
      BUSY_LOOP_MAKE_INACTIVE()
      BUSY_LOOP_CHECK_TERMINATION_THREADS()
   }
   
   // since queue pushing and state setting are done in
   // different exclusive regions, this may be needed
   set_active_if_inactive();
   ins_active;
   assert(is_active());
   assert(has_work());
   
   return true;
}

bool
threads_sched::terminate_iteration(void)
{
   START_ROUND();
   
   if(has_work())
      set_active();
   
   END_ROUND(
      more_work = num_active() > 0;
   );
}

bool
threads_sched::check_if_current_useless(void)
{
   if(!current_node->unprocessed_facts) {
      current_node->lock();
      
      if(!current_node->unprocessed_facts) {
         current_node->make_inactive();
         if(current_node->is_static() && current_node->get_static() != this) {
            // the node has changed to another scheduler!
            current_node->set_owner(current_node->get_static());
         }
         current_node->set_priority_level(0.0);
         current_node->unlock();
         current_node = NULL;
         return true;
      }
      current_node->unlock();
   }
   
   assert(current_node->unprocessed_facts);
   return false;
}

bool
threads_sched::set_next_node(void)
{
   check_priority_buffer();

   if(current_node != NULL)
      check_if_current_useless();
   
   while (current_node == NULL) {   
      if(scheduling_mechanism) {
         if(!prios.moving.empty()) {
            current_node = prios.moving.pop_best(prios.stati, STATE_WORKING);
            if(current_node) {
    //     cout << "Got node " << current_node->get_id() << " with prio " << current_node->get_priority_level() << endl;
               break;
            }
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
   tn->lock();
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this)
      do_set_node_priority(n, prio);

   tn->unlock();
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

void
threads_sched::set_node_priority_other(node *n, const double priority)
{
#ifdef SEND_OTHERS
   thread_intrusive_node *tn((thread_intrusive_node*)n);

   if(tn->has_priority_level()) {
      if(theProgram->is_priority_desc()) {
         if(tn->get_priority_level() > priority)
            // does not change
            return;
      } else {
         if(tn->get_priority_level() < priority)
            // does not change
            return;
      }
   }
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

	thread_intrusive_node *tn((thread_intrusive_node*)n);

#ifdef TASK_STEALING
   tn->lock();
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this)
      do_set_node_priority(n, tn->get_priority_level() + priority);
   else
      other->add_node_priority_other(n, priority);
   tn->unlock();
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
//   cout << "Set node " << n->get_id() << " with prio " << priority << endl;
#ifdef TASK_STEALING
   tn->lock();
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this)
      do_set_node_priority(n, priority);
   else
      other->set_node_priority_other(n, priority);
   tn->unlock();
#else
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this)
      do_set_node_priority(n, priority);
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
threads_sched::do_set_node_priority(node *n, const double priority)
{
	thread_intrusive_node *tn((thread_intrusive_node*)n);

#ifdef DEBUG_PRIORITIES
   if(priority > 0)
      tn->has_been_prioritized = true;
#endif
   if(priority < 0)
      return;

   if(current_node == tn) {
      tn->set_priority_level(priority);
      return;
   }

   if(tn->node_state() == STATE_STEALING) {
      // node is being stolen, let's simply set the level
      // and let the other thread do it's job
      if(tn->get_priority_level() != priority) {
         if(priority == 0.0)
            tn->set_priority_level(0.0);
         else {
            // we check if new priority is bigger than the current priority
            if(is_higher_priority(tn, priority))
               tn->set_priority_level(priority);
         }
      }
      return;
   }

	assert(tn->get_owner() == this);
	
   if(node_in_priority_queue(tn)) {
      // node is in the priority queues
      if(tn->get_priority_level() != priority) {
         if(priority == 0.0) {
            tn->set_priority_level(0.0);
            //cout << "Remove from frio put into main\n";
            if(tn->has_priority_level()) {
               if(is_higher_priority(tn, priority)) {
                  if(tn->node_state() == PRIORITY_MOVING)
                     prios.moving.move_node(tn, priority);
                  else {
                     prios.stati.move_node(tn, priority);
                  }
               }
            } else {
               if(tn->node_state() == PRIORITY_MOVING)
                  prios.moving.remove(tn, STATE_PRIO_CHANGE);
               else {
                  prios.stati.remove(tn, STATE_PRIO_CHANGE);
               }
               add_to_queue(tn);
            }
         } else {
            // we check if new priority is bigger than the current priority
            if(is_higher_priority(tn, priority)) {
               tn->set_priority_level(priority);
               //cout << "Moving priority\n";
               if(tn->node_state() == PRIORITY_MOVING)
                  prios.moving.move_node(tn, priority);
               else {
                  prios.stati.move_node(tn, priority);
               }
            }
         }
      }
   } else if(node_in_normal_queue(tn)) {
      // node is in the normal queue
      if(priority > 0) {
         tn->set_priority_level(priority);
         if(tn->node_state() == NORMAL_QUEUE_MOVING)
            queues.moving.remove(tn, STATE_PRIO_CHANGE);
         else if(tn->node_state() == NORMAL_QUEUE_STATIC) {
            queues.stati.remove(tn, STATE_PRIO_CHANGE);
         }
         add_to_priority_queue(tn);
      }
   } else {
#ifdef PROFILE_QUEUE
      prio_marked++;
#endif
      tn->set_priority_level(priority);
	}
	
#ifdef PROFILE_QUEUE
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

   tn->lock();
   tn->set_static(this);
   if(tn->node_state() == STATE_STEALING) {
      // node was stolen but the thief does not
      // know yet that this node is now static!
      // since the static field was set, the
      // thief knows that he needs to do something.
   } else if(node_in_normal_queue(tn)) {
      if(tn->node_state() == NORMAL_QUEUE_MOVING) {
         if(queues.moving.remove(tn, STATE_STATIC_CHANGE))
            queues.stati.push_tail(tn);
      }
   } else if(node_in_priority_queue(tn)) {
      if(tn->node_state() == PRIORITY_MOVING) {
         if(prios.moving.remove(tn, STATE_STATIC_CHANGE))
            prios.stati.insert(tn, tn->get_priority_level());
      }
   }
   tn->unlock();
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

   tn->lock();
   tn->set_moving();
   if(tn->node_state() == STATE_STEALING) {
      // node was stolen but the thief does not
      // know yet that this node is now static!
      // since the static field was set, the
      // thief knows that he needs to do something.
   } else if(node_in_normal_queue(tn)) {
      if(tn->node_state() == NORMAL_QUEUE_STATIC) {
         if(queues.stati.remove(tn, STATE_STATIC_CHANGE))
            queues.moving.push_tail(tn);
      }
   } else if(node_in_priority_queue(tn)) {
      if(tn->node_state() == PRIORITY_STATIC) {
         if(prios.stati.remove(tn, STATE_STATIC_CHANGE))
            prios.moving.insert(tn, tn->get_priority_level());
      }
   }
   tn->unlock();
}

void
threads_sched::move_node_to_new_owner(thread_intrusive_node *tn, threads_sched *new_owner)
{
   new_owner->add_to_queue(tn);

   lock_guard<mutex> l2(new_owner->lock);
   
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

   tn->lock();
   tn->set_static(new_owner);
   if(tn->get_owner() == new_owner) {
   } else if(tn->node_state() == STATE_STEALING) {
      // node is being stolen right now!
      // change both owner and is_static
      tn->set_owner(new_owner);
   } else if(node_in_normal_queue(tn)) {
      const queue_id_t state(tn->node_state());
      if(state == NORMAL_QUEUE_STATIC) {
         if(queues.stati.remove(tn, STATE_AFFINITY_CHANGE)) {
            tn->set_owner(new_owner);
            move_node_to_new_owner(tn, new_owner);
         } else {
            // the node is now being processed by the scheduler!
         }
      } else if(state == NORMAL_QUEUE_MOVING) {
         if(queues.moving.remove(tn, STATE_AFFINITY_CHANGE)) {
            tn->set_owner(new_owner);
            move_node_to_new_owner(tn, new_owner);
         } else {
            // node is being processed by the scheduler.
         }
      } else {
         // node is being processed by the scheduler.
      }
   } else if(node_in_priority_queue(tn)) {
      const queue_id_t state(tn->node_state());

      if(state == PRIORITY_MOVING) {
         if(prios.moving.remove(tn, STATE_AFFINITY_CHANGE)) {
            tn->set_owner(new_owner);
            move_node_to_new_owner(tn, new_owner);
         } else {
            // the node is now being processed by the scheduler!
         }
      } else if(state == PRIORITY_STATIC) {
         if(prios.stati.remove(tn, STATE_AFFINITY_CHANGE)) {
            tn->set_owner(new_owner);
            move_node_to_new_owner(tn, new_owner);
         } else {
            // the node is now being processed by the scheduler!
         }
      } else {
         // node is being processed by the scheduler.
      }
   } else if(tn->node_state() == STATE_WORKING) {
      // we set static to the new owner
      // the scheduler using that node right now will
      // change its owner once it finishes processing the node
   } else
      tn->set_owner(new_owner);

   tn->unlock();
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
   sent_facts_same_thread = 0;
   sent_facts_other_thread = 0;
   sent_facts_other_thread_now = 0;
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
   current_node(NULL)
#ifdef TASK_STEALING
   , rand(time(NULL) + _id * 10)
   , next_thread(rand(All->NUM_THREADS))
   , backoff(STEALING_ROUND_MAX)
#endif
#ifdef INSTRUMENTATION
   , sent_facts_same_thread(0)
   , sent_facts_other_thread(0)
   , sent_facts_other_thread_now(0)
#ifdef TASK_STEALING
   , stolen_total(0)
#endif
#endif
   , priority_buffer(std::min(PRIORITY_BUFFER_SIZE,
                     vm::All->DATABASE->num_nodes() / vm::All->NUM_THREADS))
{
}

threads_sched::~threads_sched(void)
{
}
   
}

