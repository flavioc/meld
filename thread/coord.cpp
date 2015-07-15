
#include "thread/thread.hpp"
#include "interface.hpp"

using namespace db;
using namespace vm;
using namespace std;

namespace sched
{

void
thread::schedule_next(node *n)
{
   const priority_t prio(max_priority_value());

   //cout << "Schedule next " << n->get_id() << endl;

#ifdef TASK_STEALING
   LOCK_STACK(nodelock);
   NODE_LOCK(n, nodelock, schedule_next_lock);

   thread *other(n->get_owner());
   if(other == this)
      do_set_node_priority(n, prio, true);

   NODE_UNLOCK(n, nodelock);
#else
   do_set_node_priority(n, prio, true);
#endif
}

void
thread::add_node_priority_other(node *n, const priority_t priority)
{
#ifdef SEND_OTHERS
   thread *other(n->get_owner());

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
thread::check_priority_buffer(void)
{
#ifdef SEND_OTHERS
   if(priority_buffer.size() == 0)
      return;

   size_t size(priority_buffer.pop(priority_tmp));

   for(size_t i(0); i < size; ++i) {
      priority_add_item p(priority_tmp[i]);
      node *target(p.target);
      if(target == nullptr)
         continue;
      const priority_t howmuch(p.val);
      const priority_add_type typ(p.typ);
      if(target->get_owner() != this)
         continue;
#ifdef TASK_STEALING
      LOCK_STACK(nodelock);
      NODE_LOCK(target, nodelock);
		if(target->get_owner() == this) {
         switch(typ) {
            case ADD_PRIORITY:
               do_set_node_priority(target, target->get_priority() + howmuch);
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
thread::set_node_priority_other(node *n, const priority_t priority)
{
#ifdef SEND_OTHERS
   // will potentially change
   thread *other(n->get_owner());
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
thread::add_node_priority(node *tn, const priority_t priority)
{
   if(!scheduling_mechanism)
      return;

   if(current_node == tn)
      return;

#ifdef TASK_STEALING
   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock, add_priority_lock);
   thread *other(tn->get_owner());
   if(other == this)
      do_set_node_priority(tn, tn->get_priority() + priority);
   else
#ifdef DIRECT_PRIORITIES
      do_set_node_priority_other(tn, tn->get_priority() + priority);
#else
      other->add_node_priority_other(tn, priority);
#endif
   NODE_UNLOCK(tn, nodelock);
#else
   thread *other(tn->get_owner());
   if(other == this) {
      const priority_t old_prio(tn->get_priority());
      set_node_priority(tn, old_prio + priority);
   }
#endif
}

void
thread::set_default_node_priority(node *tn, const priority_t priority)
{
   if(!scheduling_mechanism)
      return;

   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock, add_priority_lock);
//   cout << "Default priority " << priority << endl;
   tn->set_default_priority(priority);
   NODE_UNLOCK(tn, nodelock);
}

void
thread::do_remove_node_priority(node *tn)
{
   switch(tn->node_state()) {
      case STATE_STEALING:
         // node is being stolen, remove priority
         // and let the other thread do it's job
         tn->remove_temporary_priority();
         break;
      case PRIORITY_MOVING:
         tn->remove_temporary_priority();
         if(prios.moving.remove(tn, STATE_PRIO_CHANGE LOCKING_STAT_FLAG_FALSE))
            queues.moving.push_tail(tn LOCKING_STAT_FLAG_FALSE);
         break;
      case PRIORITY_STATIC:
         tn->remove_temporary_priority();
         if(prios.stati.remove(tn, STATE_PRIO_CHANGE))
            queues.stati.push_tail(tn LOCKING_STAT_FLAG_FALSE);
         break;
      case NORMAL_QUEUE_MOVING: // already without priority
      case NORMAL_QUEUE_STATIC:
         tn->remove_temporary_priority();
         break;
      case STATE_WORKING:
         tn->remove_temporary_priority();
         break;
      case STATE_IDLE:
         tn->remove_temporary_priority();
         break;
      default: abort(); break;
   }
}

void
thread::do_remove_node_priority_other(node *tn)
{
   // we know that the owner of node is not this.
   queue_id_t state(tn->node_state());
   thread *owner(tn->get_owner());

   switch(state) {
      case STATE_STEALING:
         // node is being stolen.
         tn->remove_temporary_priority();
         break;
      case NORMAL_QUEUE_MOVING:
      case NORMAL_QUEUE_STATIC:
         tn->remove_temporary_priority();
         break;
      case PRIORITY_MOVING:
         tn->remove_temporary_priority();
         if(owner->prios.moving.remove(tn, STATE_PRIO_CHANGE LOCKING_STAT_FLAG_FALSE)) {
            owner->queues.moving.push_tail(tn LOCKING_STAT_FLAG_FALSE);
            owner->activate_thread();
         }
         break;
      case PRIORITY_STATIC:
         tn->remove_temporary_priority();
         if(owner->prios.stati.remove(tn, STATE_PRIO_CHANGE LOCKING_STAT_FLAG_FALSE)) {
            owner->queues.stati.push_tail(tn LOCKING_STAT_FLAG_FALSE);
            owner->activate_thread();
         }
         break;
      case STATE_WORKING:
         // do nothing
         break;
      default: abort(); break;
   } 
}

void
thread::remove_node_priority(node *tn)
{
   if(!scheduling_mechanism)
      return;

   //cout << "Remove priority " << tn->get_id() << endl;

   if(current_node == tn) {
      tn->remove_temporary_priority();
      return;
   }

#ifdef TASK_STEALING
   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock, set_priority_lock);
   thread *other(tn->get_owner());
   if(other == this)
      do_remove_node_priority(tn);
   else
#ifdef DIRECT_PRIORITIES
      do_remove_node_priority_other(tn);
#else
      { } // do nothing
#endif
   NODE_UNLOCK(tn, nodelock);
#else
   thread *other(tn->get_owner());
   if(other == this)
      do_remove_node_priority(tn);
#endif

}

void
thread::do_set_node_priority_other(db::node *node, const priority_t priority)
{
#ifdef INSTRUMENTATION
   priority_nodes_others++;
#endif
   // we know that the owner of node is not this.
   queue_id_t state(node->node_state());
   thread *owner(node->get_owner());

   switch(state) {
      case STATE_STEALING:
         // node is being stolen.
         node->set_temporary_priority_if(priority);
         break;
      case NORMAL_QUEUE_MOVING:
         node->set_temporary_priority_if(priority);
         if(owner->queues.moving.remove(node, STATE_PRIO_CHANGE LOCKING_STAT_FLAG_FALSE)) {
            owner->prios.moving.insert(node, node->get_priority() LOCKING_STAT_FLAG_FALSE);
            owner->activate_thread();
         }
         break;
      case NORMAL_QUEUE_STATIC:
         node->set_temporary_priority_if(priority);
         if(owner->queues.stati.remove(node, STATE_PRIO_CHANGE LOCKING_STAT_FLAG_FALSE)) {
            owner->prios.stati.insert(node, node->get_priority() LOCKING_STAT_FLAG_FALSE);
            owner->activate_thread();
         }
         break;
      case PRIORITY_MOVING:
         if(node->set_temporary_priority_if(priority))
            owner->prios.moving.move_node(node, priority LOCKING_STAT_FLAG_FALSE);
         break;
      case PRIORITY_STATIC:
         if(node->set_temporary_priority_if(priority))
            owner->prios.stati.move_node(node, priority LOCKING_STAT_FLAG_FALSE);
         break;
      case STATE_PRIO_CHANGE:
      case STATE_IDLE:
         node->set_temporary_priority_if(priority);
         break;
      case STATE_WORKING:
         // do nothing
         break;
      default: abort(); break;
   } 
}

void
thread::set_node_static(db::node *tn)
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
            prios.stati.insert(tn, tn->get_priority() LOCKING_STAT_FLAG_FALSE);
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
thread::set_node_moving(db::node *tn)
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
            prios.moving.insert(tn, tn->get_priority() LOCKING_STAT_FLAG_FALSE);
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
thread::move_node_to_new_owner(db::node *tn, thread *new_owner)
{
   // this must be called while helding the node lock.
   tn->just_moved_buffer();
   new_owner->add_to_queue(tn);
   comm_threads.set_bit(new_owner->get_id());
}

void
thread::set_node_cpu(db::node *node, sched::thread *new_owner)
{
   if(!scheduling_mechanism)
      return;

   set_node_owner(node, new_owner);
}

void
thread::set_node_owner(db::node *tn, thread *new_owner)
{
   LOCK_STACK(nodelock);

   if(tn == current_node) {
      // we will change the owner field once we are done with the node.
      NODE_LOCK(tn, nodelock, set_affinity_lock);
      if(new_owner == this)
         tn->just_moved();
      make_node_static(tn, new_owner);
      NODE_UNLOCK(tn, nodelock);
      return;
   }

   NODE_LOCK(tn, nodelock, set_affinity_lock);
   make_node_static(tn, new_owner);
   if(tn->get_owner() == new_owner) {
      tn->just_moved_buffer();
      NODE_UNLOCK(tn, nodelock);
      return;
   }

   bool done = false;
   while (!done) {
      switch(tn->node_state()) {
         case STATE_STEALING:
            // node is being stolen right now!
            // change both owner and is_static
            tn->set_owner(new_owner);
            done = true;
            break;
         case NORMAL_QUEUE_STATIC:
            if(queues.stati.remove(tn, STATE_AFFINITY_CHANGE LOCKING_STAT_FLAG_FALSE)) {
               tn->set_owner(new_owner);
               move_node_to_new_owner(tn, new_owner);
               done = true;
            }
            break;
         case NORMAL_QUEUE_MOVING:
            if(queues.moving.remove(tn, STATE_AFFINITY_CHANGE LOCKING_STAT_FLAG_FALSE)) {
               tn->set_owner(new_owner);
               move_node_to_new_owner(tn, new_owner);
               done = true;
            }
            break;
         case PRIORITY_MOVING:
            if(prios.moving.remove(tn, STATE_AFFINITY_CHANGE LOCKING_STAT_FLAG_FALSE)) {
               tn->set_owner(new_owner);
               move_node_to_new_owner(tn, new_owner);
               done = true;
            }
            break;
         case PRIORITY_STATIC:
            if(prios.stati.remove(tn, STATE_AFFINITY_CHANGE LOCKING_STAT_FLAG_FALSE)) {
               tn->set_owner(new_owner);
               move_node_to_new_owner(tn, new_owner);
               done = true;
            }
            break;
         case STATE_WORKING:
            // we set static to the new owner
            // the scheduler using that node right now will
            // change its owner once it finishes processing the node
            done = true;
            break;
         case STATE_IDLE:
            tn->set_owner(new_owner);
            tn->just_moved_buffer();
            done = true;
            break;
         default: assert(false); break; 
      }
   }

   NODE_UNLOCK(tn, nodelock);
}

void
thread::set_node_affinity(db::node *node, db::node *affinity)
{
   if(!scheduling_mechanism)
      return;
   thread *new_owner(affinity->get_owner());
   set_node_owner(node, new_owner);
}

}
