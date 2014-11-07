
#include "thread/threads.hpp"
#include "interface.hpp"

using namespace db;
using namespace vm;
using namespace std;

namespace sched
{

static inline bool
is_higher_priority(thread_intrusive_node *tn, const double priority)
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
      LOCK_STACK(nodelock);
      NODE_LOCK(tn, nodelock);
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
      NODE_UNLOCK(tn, nodelock);
#endif
   }
#endif
}
#endif

void
threads_sched::set_node_priority_other(node *n, const double priority)
{
#ifdef SEND_OTHERS
   thread_intrusive_node *tn((thread_intrusive_node*)n);

   // will potentially change
   threads_sched *other((threads_sched*)tn->get_owner());
   priority_add_item item;
   item.typ = SET_PRIORITY;
   item.val = priority;
   item.target = tn;

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
   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock);
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
   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock);
   LOCK_STAT(prio_lock);
   if(!is_higher_priority(tn, priority) && priority != 0) {
      NODE_UNLOCK(tn, nodelock);
      return;
   }
   threads_sched *other((threads_sched*)tn->get_owner());
   if(other == this)
      do_set_node_priority(n, priority);
   else
#ifdef DIRECT_PRIORITIES
      do_set_node_priority_other(tn, priority);
#else
      other->set_node_priority_other(n, priority);
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
threads_sched::do_set_node_priority_other(thread_intrusive_node *node, const double priority)
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
      utils::lock_guard l2(owner->lock);
      
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
threads_sched::set_node_static(db::node *n)
{
   if(!scheduling_mechanism)
      return;

   //cout << "Static " << n->get_id() << endl;

   thread_intrusive_node *tn(static_cast<thread_intrusive_node*>(n));
   if(n == current_node) {
      make_node_static(tn, this);
      return;
   }

   if(tn->is_static())
      return;

   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock);
   make_node_static(tn, this);
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
   NODE_UNLOCK(tn, nodelock);
}

void
threads_sched::set_node_moving(db::node *n)
{
   if(!scheduling_mechanism)
      return;

//   cout << "Moving " << n->get_id() << endl;

   thread_intrusive_node *tn(static_cast<thread_intrusive_node*>(n));
   if(n == current_node) {
      make_node_moving(tn);
      return;
   }

   if(tn->is_moving())
      return;

   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock);
   make_node_moving(tn);
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
   NODE_UNLOCK(tn, nodelock);
}

void
threads_sched::move_node_to_new_owner(thread_intrusive_node *tn, threads_sched *new_owner)
{
   new_owner->add_to_queue(tn);

   utils::lock_guard l2(new_owner->lock);
   
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
threads_sched::set_node_owner(db::node *node, threads_sched *new_owner)
{
   thread_intrusive_node *tn(static_cast<thread_intrusive_node*>(node));

   if(tn == current_node) {
      // we will change the owner field once we are done with the node.
      make_node_static(tn, new_owner);
      return;
   }

   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock);
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
