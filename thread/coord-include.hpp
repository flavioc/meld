
inline void
do_set_node_priority(db::node *tn, const vm::priority_t priority, const bool force_queue = false)
{
#ifdef INSTRUMENTATION
   priority_nodes_thread++;
#endif

   if(current_node == tn) {
      tn->set_temporary_priority(priority);
      return;
   }

   switch(tn->node_state()) {
      case STATE_STEALING:
         // node is being stolen, let's simply set the level
         // and let the other thread do it's job
         tn->set_temporary_priority_if(priority);
         (void)force_queue; // it's already on the queue!
         break;
      case PRIORITY_MOVING:
         if(tn->set_temporary_priority_if(priority))
            prios.moving.move_node(tn, priority LOCKING_STAT_FLAG_FALSE);
         break;
      case PRIORITY_STATIC:
         if(tn->set_temporary_priority_if(priority))
            prios.stati.move_node(tn, priority LOCKING_STAT_FLAG_FALSE);
         break;
      case NORMAL_QUEUE_MOVING:
         tn->set_temporary_priority_if(priority);
         if(queues.moving.remove(tn, STATE_PRIO_CHANGE LOCKING_STAT_FLAG_FALSE)) {
            prios.moving.insert(tn, tn->get_priority());
         }
         break;
      case NORMAL_QUEUE_STATIC:
         tn->set_temporary_priority_if(priority);
         if(queues.stati.remove(tn, STATE_PRIO_CHANGE))
            prios.stati.insert(tn, tn->get_priority());
         break;
      case STATE_WORKING:
      case STATE_PRIO_CHANGE:
         tn->set_temporary_priority_if(priority);
         break;
      case STATE_IDLE:
         tn->set_temporary_priority_if(priority);
         if(force_queue)
            add_to_queue(tn);
         break;
      default: abort(); break;
   }
#ifdef PROFILE_QUEUE
   prio_marked++;
	prio_count++;
#endif
}

inline void
set_node_priority(db::node *tn, const vm::priority_t priority)
{
   //cout << "Set node " << tn->get_id() << " with prio " << priority << endl;
   if(current_node == tn)
      return;

#ifdef TASK_STEALING
   LOCK_STACK(nodelock);
   NODE_LOCK(tn, nodelock, set_priority_lock);
   if(!is_higher_priority(priority, tn)) {
      NODE_UNLOCK(tn, nodelock);
      return;
   }
   thread *other(tn->get_owner());
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
   if(!is_higher_priority(priority, tn))
      return;
   sched::thread *other(tn->get_owner());
   if(other == this)
      do_set_node_priority(tn, priority);
#endif
}

