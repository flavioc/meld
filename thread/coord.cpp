
#include "thread/threads.hpp"
#include "interface.hpp"

using namespace db;
using namespace vm;
using namespace std;

namespace sched
{

static inline bool
is_higher_priority(db::node *tn, const priority_t priority)
{
   if(theProgram->is_priority_desc())
      return tn->get_priority_level() < priority;
   else {
      const double old(tn->get_priority_level());
      if(old == 0)
         return priority > 0;
      return old > priority;
   }
}

void
threads_sched::schedule_next(node *n)
{
   static const double add = 100.0;
   double prio(0.0);

   if(!prios.moving.empty())
      prio = prios.moving.min_value();
   if(theProgram->is_priority_desc()) {
      if(!prios.stati.empty())
         prio = max(prio, prios.stati.min_value());

      prio += add;
   } else {
      if(!prios.stati.empty())
         prio = min(prio, prios.stati.min_value());

      prio -= add;
   }

#ifdef TASK_STEALING
   LOCK_STACK(nodelock);
   NODE_LOCK(n, nodelock, schedule_next_lock);
   threads_sched *other((threads_sched*)n->get_owner());
   if(other == this)
      do_set_node_priority(n, prio);

   NODE_UNLOCK(n, nodelock);
#else
   do_set_node_priority(n, prio);
#endif
}

void
threads_sched::add_node_priority_other(node *n, const double priority)
{
#ifdef SEND_OTHERS
   threads_sched *other((threads_sched*)n->get_owner());

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
      if(target == NULL)
         continue;
      const double howmuch(p.val);
      priority_add_type typ(p.typ);
      if(target->get_owner() != this)
         continue;
#ifdef TASK_STEALING
      LOCK_STACK(nodelock);
      NODE_LOCK(target, nodelock);
		if(target->get_owner() == this) {
         switch(typ) {
            case ADD_PRIORITY:
               do_set_node_priority(target, target->get_priority_level() + howmuch);
               break;
            case SET_PRIORITY:
               do_set_node_priority(target, howmuch);
               break;
         }
      }
      NODE_UNLOCK(target, nodelock);
#endif
   }
#endif
}
#endif

void
threads_sched::set_node_priority_other(node *n, const double priority)
{
#ifdef SEND_OTHERS
   // will potentially change
   threads_sched *other((threads_sched*)n->get_owner());
   priority_add_item item;
   item.typ = SET_PRIORITY;
   item.val = priority;
   item.target = n;

   other->priority_buffer.add(item);
#else
   (void)priority;
   (void)n;
#endif
}

void
threads_sched::add_node_priority(node *tn, const double priority)
{
   if(!scheduling_mechanism)
      return;
#ifdef FACT_STATISTICS
   count_add_priority++;
#endif

#ifdef TASK_STEALING
   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock, add_priority_lock);
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this)
      do_set_node_priority(tn, tn->get_priority_level() + priority);
   else
#ifdef DIRECT_PRIORITIES
      do_set_node_priority_other(tn, tn->get_priority_level() + priority);
#else
      other->add_node_priority_other(tn, priority);
#endif
   NODE_UNLOCK(tn, nodelock);
#else
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this) {
      const double old_prio(tn->get_priority_level());
      set_node_priority(n, old_prio + priority);
   }
#endif
}

void
threads_sched::set_default_node_priority(node *tn, const double priority)
{
   if(!scheduling_mechanism)
      return;

//   cout << "Default priority " << priority << endl;
   tn->set_default_priority_level(priority);
}

void
threads_sched::set_node_priority(node *tn, const double priority)
{
   if(!scheduling_mechanism)
      return;

#ifdef FACT_STATISTICS
   count_set_priority++;
#endif

//   cout << "Set node " << n->get_id() << " with prio " << priority << endl;
#ifdef TASK_STEALING
   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock, set_priority_lock);
   if(!is_higher_priority(tn, priority) && priority != 0) {
      NODE_UNLOCK(tn, nodelock);
      return;
   }
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this)
      do_set_node_priority(tn, priority);
   else
#ifdef DIRECT_PRIORITIES
      do_set_node_priority_other(tn, priority);
#else
      other->set_node_priority_other(tn, priority);
#endif
   NODE_UNLOCK(tn, nodelock);
#else
   if(!is_higher_priority(tn, priority) && priority != 0)
      return;
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this)
      do_set_node_priority(n, priority);
#endif
}

void
threads_sched::do_set_node_priority_other(db::node *node, const double priority)
{
#ifdef INSTRUMENTATION
   priority_nodes_others++;
#endif
   // we know that the owner of node is not this.
   queue_id_t state(node->node_state());
   threads_sched *owner(static_cast<threads_sched*>(node->get_owner()));
   bool activate_node(false);

   switch(state) {
      case STATE_STEALING:
         // node is being stolen.
         if(is_higher_priority(node, priority))
            node->set_priority_level(priority);
         return;
      case NORMAL_QUEUE_MOVING:
         if(owner->queues.moving.remove(node, STATE_PRIO_CHANGE LOCKING_STAT_FLAG_FALSE)) {
            owner->prios.moving.insert(node, node->get_priority_level() LOCKING_STAT_FLAG_FALSE);
            activate_node = true;
         }
         break;
      case NORMAL_QUEUE_STATIC:
         if(owner->queues.stati.remove(node, STATE_PRIO_CHANGE LOCKING_STAT_FLAG_FALSE)) {
            owner->prios.stati.insert(node, node->get_priority_level() LOCKING_STAT_FLAG_FALSE);
            activate_node = true;
         }
         break;
      case PRIORITY_MOVING:
         if(priority == 0) {
            node->set_priority_level(0.0);
            if(owner->prios.moving.remove(node, STATE_PRIO_CHANGE LOCKING_STAT_FLAG_FALSE)) {
               owner->queues.moving.push_tail(node LOCKING_STAT_FLAG_FALSE);
               activate_node = true;
            }
         } else if(is_higher_priority(node, priority)) {
            node->set_priority_level(priority);
            owner->prios.moving.move_node(node, priority LOCKING_STAT_FLAG_FALSE);
         }
         break;
      case PRIORITY_STATIC:
         if(priority == 0) {
            node->set_priority_level(0.0);
            if(owner->prios.stati.remove(node, STATE_PRIO_CHANGE LOCKING_STAT_FLAG_FALSE)) {
               owner->queues.stati.push_tail(node LOCKING_STAT_FLAG_FALSE);
               activate_node = true;
            }
         } else if(is_higher_priority(node, priority)) {
            node->set_priority_level(priority);
            owner->prios.stati.move_node(node, priority LOCKING_STAT_FLAG_FALSE);
         }
         break;
      case STATE_WORKING:
         // do nothing
         break;
      default: assert(false); break;
   } 
   if(activate_node) {
      // we must reactivate node if it's not active
      MUTEX_LOCK_GUARD(owner->lock, thread_lock);

      if(owner->is_inactive()) {
         owner->set_active();
         assert(owner->is_active());
      }
   }
}

void
threads_sched::do_set_node_priority(node *tn, const double priority)
{
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
            if(prios.moving.remove(tn, STATE_PRIO_CHANGE LOCKING_STAT_FLAG_FALSE))
               queues.moving.push_tail(tn LOCKING_STAT_FLAG_FALSE);
         } else if(is_higher_priority(tn, priority)) {
            // we check if new priority is bigger than the current priority
            tn->set_priority_level(priority);
            prios.moving.move_node(tn, priority LOCKING_STAT_FLAG_FALSE);
         }
         break;
      case PRIORITY_STATIC:
         if(priority == 0.0) {
            tn->set_priority_level(0.0);
            if(prios.stati.remove(tn, STATE_PRIO_CHANGE))
               queues.stati.push_tail(tn LOCKING_STAT_FLAG_FALSE);
         } else if(is_higher_priority(tn, priority)) {
            // we check if new priority is bigger than the current priority
            tn->set_priority_level(priority);
            prios.stati.move_node(tn, priority LOCKING_STAT_FLAG_FALSE);
         }
         break;
      case NORMAL_QUEUE_MOVING:
         tn->set_priority_level(priority);
         if(queues.moving.remove(tn, STATE_PRIO_CHANGE LOCKING_STAT_FLAG_FALSE))
            prios.moving.insert(tn, tn->get_priority_level());
         break;
      case NORMAL_QUEUE_STATIC:
         tn->set_priority_level(priority);
         if(queues.stati.remove(tn, STATE_PRIO_CHANGE))
            prios.stati.insert(tn, tn->get_priority_level());
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
threads_sched::set_node_static(db::node *tn)
{
   if(!scheduling_mechanism)
      return;

   //cout << "Static " << n->get_id() << endl;

   if(tn == current_node) {
      make_node_static(tn, this);
      return;
   }

   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock, set_static_lock);

   if(tn->is_static()) {
      NODE_UNLOCK(tn, nodelock);
      return;
   }

   make_node_static(tn, this);
   switch(tn->node_state()) {
      case STATE_STEALING:
         // node was stolen but the thief does not
         // know yet that this node is now static!
         // since the static field was set, the
         // thief knows that he needs to do something.
         break;
      case NORMAL_QUEUE_MOVING:
         if(queues.moving.remove(tn, STATE_STATIC_CHANGE LOCKING_STAT_FLAG_FALSE))
            queues.stati.push_tail(tn LOCKING_STAT_FLAG_FALSE);
         break;
      case PRIORITY_MOVING:
         if(prios.moving.remove(tn, STATE_STATIC_CHANGE LOCKING_STAT_FLAG_FALSE))
            prios.stati.insert(tn, tn->get_priority_level() LOCKING_STAT_FLAG_FALSE);
         break;
      case NORMAL_QUEUE_STATIC:
      case PRIORITY_STATIC:
      case STATE_IDLE:
      case STATE_WORKING:
         // do nothing
         break;
      default: assert(false); break;
   }
   NODE_UNLOCK(tn, nodelock);
}

void
threads_sched::set_node_moving(db::node *tn)
{
   if(!scheduling_mechanism)
      return;

//   cout << "Moving " << n->get_id() << endl;

   if(tn == current_node) {
      make_node_moving(tn);
      return;
   }

   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock, set_moving_lock);
   if(tn->is_moving()) {
      NODE_UNLOCK(tn, nodelock);
      return;
   }

   make_node_moving(tn);
   switch(tn->node_state()) {
      case STATE_STEALING:
         // node was stolen but the thief does not
         // know yet that this node is now static!
         // since the static field was set, the
         // thief knows that he needs to do something.
         break;
      case NORMAL_QUEUE_STATIC:
         if(queues.stati.remove(tn, STATE_STATIC_CHANGE LOCKING_STAT_FLAG_FALSE))
            queues.moving.push_tail(tn LOCKING_STAT_FLAG_FALSE);
         break;
      case PRIORITY_STATIC:
         if(prios.stati.remove(tn, STATE_STATIC_CHANGE LOCKING_STAT_FLAG_FALSE))
            prios.moving.insert(tn, tn->get_priority_level() LOCKING_STAT_FLAG_FALSE);
         break;
      case NORMAL_QUEUE_MOVING:
      case PRIORITY_MOVING:
      case STATE_WORKING:
      case STATE_IDLE:
         // do nothing
         break;
      default: assert(false); break;
   }
   NODE_UNLOCK(tn, nodelock);
}

void
threads_sched::move_node_to_new_owner(db::node *tn, threads_sched *new_owner)
{
   new_owner->add_to_queue(tn);

   MUTEX_LOCK_GUARD(new_owner->lock, thread_lock);
   
   if(new_owner->is_inactive())
   {
      new_owner->set_active();
      assert(new_owner->is_active());
   }
}

void
threads_sched::set_node_cpu(db::node *node, const int_val val)
{
   if(!scheduling_mechanism)
      return;

   if(val >= (int_val)All->NUM_THREADS)
      return;

   threads_sched *new_owner(static_cast<threads_sched*>(All->SCHEDS[val]));

   set_node_owner(node, new_owner);
}

void
threads_sched::set_node_owner(db::node *tn, threads_sched *new_owner)
{
   if(tn == current_node) {
      // we will change the owner field once we are done with the node.
      make_node_static(tn, new_owner);
      return;
   }

   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock, set_affinity_lock);
   make_node_static(tn, new_owner);
   if(tn->get_owner() == new_owner) {
      NODE_UNLOCK(tn, nodelock);
      return;
   }

   switch(tn->node_state()) {
      case STATE_STEALING:
         // node is being stolen right now!
         // change both owner and is_static
         tn->set_owner(new_owner);
         break;
      case NORMAL_QUEUE_STATIC:
         if(queues.stati.remove(tn, STATE_AFFINITY_CHANGE LOCKING_STAT_FLAG_FALSE)) {
            tn->set_owner(new_owner);
            move_node_to_new_owner(tn, new_owner);
         }
         break;
      case NORMAL_QUEUE_MOVING:
         if(queues.moving.remove(tn, STATE_AFFINITY_CHANGE LOCKING_STAT_FLAG_FALSE)) {
            tn->set_owner(new_owner);
            move_node_to_new_owner(tn, new_owner);
         }
         break;
      case PRIORITY_MOVING:
         if(prios.moving.remove(tn, STATE_AFFINITY_CHANGE LOCKING_STAT_FLAG_FALSE)) {
            tn->set_owner(new_owner);
            move_node_to_new_owner(tn, new_owner);
         }
         break;
      case PRIORITY_STATIC:
         if(prios.stati.remove(tn, STATE_AFFINITY_CHANGE LOCKING_STAT_FLAG_FALSE)) {
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

   NODE_UNLOCK(tn, nodelock);
}

void
threads_sched::set_node_affinity(db::node *node, db::node *affinity)
{
   if(!scheduling_mechanism)
      return;
   threads_sched *new_owner(static_cast<threads_sched*>(affinity->get_owner()));
   set_node_owner(node, new_owner);
}

}
