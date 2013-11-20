#include <iostream>
#include <boost/thread/barrier.hpp>

#include "thread/prio.hpp"
#include "db/database.hpp"
#include "db/tuple.hpp"
#include "process/remote.hpp"
#include "sched/thread/assert.hpp"
#include "sched/common.hpp"
#include "utils/time.hpp"

using namespace boost;
using namespace std;
using namespace process;
using namespace vm;
using namespace db;
using namespace utils;

//#define DEBUG_PRIORITIES
//#define PROFILE_QUEUE

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

db::database *prio_db(NULL);

void
threads_prio::assert_end(void) const
{
   assert(priority_buffer.empty());
	threads_sched::assert_end();
}

void
threads_prio::assert_end_iteration(void) const
{
	threads_sched::assert_end_iteration();
   assert(priority_buffer.empty());
}

#ifdef TASK_STEALING

void
threads_prio::check_stolen_node(thread_intrusive_node *tn)
{
   if(priority_queue::in_queue(tn)) {
      prio_queue.remove(tn);
   } else if(node_queue::in_queue(tn)) {
      queue_nodes.remove(tn);
   }
}

thread_intrusive_node*
threads_prio::steal_node(void)
{
   thread_intrusive_node *node(NULL);

   if(steal_flag) {
      if(!queue_nodes.empty())
         queue_nodes.pop(node);
      else if(!prio_queue.empty())
         node = prio_queue.pop();
      steal_flag = false;
   } else {
      if(!prio_queue.empty())
         node = prio_queue.pop();
      else if(!queue_nodes.empty())
         queue_nodes.pop(node);
      steal_flag = true;
   }
   return node;
}
#endif

void
threads_prio::check_priority_buffer(void)
{
#if 0
   while(!priority_buffer.empty()) {
      priority_buffer.pop();
   }

   return;
#else

   while(!priority_buffer.empty()) {
      priority_add_item p(priority_buffer.pop());
      node *target(p.target);
      thread_intrusive_node *tn((thread_intrusive_node *)target);
      double howmuch(p.val);
      priority_add_type typ(p.typ);

		if(tn->get_owner() != this) {
         threads_prio *other((threads_prio*)tn->get_owner());
         other->priority_buffer.push(p);
			continue; // skip nodes we do not own
      }

      node_priorities::iterator it(node_map.find(target));
      if (it == node_map.end()) {
         switch(typ) {
            case ADD_PRIORITY:
               node_map[target] = tn->get_float_priority_level() + howmuch;
               break;
            case SET_PRIORITY:
               node_map[target] = howmuch;
               break;
            default:
               assert(false);
         }
      } else {
         const double oldval(it->second);
         switch(typ) {
            case ADD_PRIORITY:
               node_map[target] = oldval + oldval;
               break;
            case SET_PRIORITY:
               node_map[target] = howmuch;
               break;
         }
      }
   }

   // go through the map and set the priority
   for(node_priorities::iterator it(node_map.begin()), end(node_map.end());
         it != end;
         ++it)
   {
      node *target(it->first);
      const double priority(it->second);

      set_node_priority(target, priority);
   }

   node_map.clear();
#endif
}

bool
threads_prio::check_if_current_useless(void)
{
	assert(current_node->in_queue());
	
	if(!prio_queue.empty() && !current_node->has_work())
   {
      current_node->lock();
      current_node->set_in_queue(false);
      switch(state.all->PROGRAM->get_priority_type()) {
         case FIELD_INT:
            current_node->set_int_priority_level(0);
            break;
         case FIELD_FLOAT:
            current_node->set_float_priority_level(0.0);
            break;
         default:
            assert(false);
            break;
      }
      assert(!current_node->in_queue());
      assert(!priority_queue::in_queue(current_node));
      current_node->unlock();
      current_node = prio_queue.pop();
      //cout << "Prio: " << current_node->get_float_priority_level() << endl;
      taken_from_priority_queue = true;
      return false;
	} else if(!current_node->has_work()) {
      
      current_node->lock();
      current_node->set_in_queue(false);
      switch(state.all->PROGRAM->get_priority_type()) {
         case FIELD_INT:
            current_node->set_int_priority_level(0);
            break;
         case FIELD_FLOAT:
            current_node->set_float_priority_level(0.0);
            break;
         default:
            assert(false);
            break;
      }
      assert(!current_node->in_queue());
      current_node->unlock();
      current_node = NULL;
      return true;
   }
   
   assert(current_node->has_work());
   return false;
}

bool
threads_prio::set_next_node(void)
{
   if(current_node != NULL)
      check_if_current_useless();
   
   while (current_node == NULL) {   
      check_priority_buffer();
		
      if(!has_work()) {
         if(!busy_wait())
            return false;
      }
      
      assert(has_work());

		if(!prio_queue.empty()) {
			current_node = prio_queue.pop();
         //cout << "Picked node " << current_node->get_id() << " with " << current_node->get_int_priority_level() << endl;
         taken_from_priority_queue = true;
			goto loop_check;
		} else if(!queue_nodes.empty()) {
			const bool suc(queue_nodes.pop(current_node));
			if(!suc)
				continue;
         taken_from_priority_queue = false;
		} else
         continue;

loop_check:
      assert(current_node != NULL);
      assert(current_node->in_queue());

      check_if_current_useless();
   }
   
   ins_active;
   
   assert(current_node != NULL);
   
   return true;
}

node*
threads_prio::get_work(void)
{
   check_priority_buffer();
	
   if(!set_next_node())
      return NULL;

   set_active_if_inactive();
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(current_node->has_work());
   
   return current_node;
}

void
threads_prio::end(void)
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
threads_prio::schedule_next(node *n)
{
   static const double add = 100.0;
   double prio(0.0);
   if(prio_queue.empty()) {
      prio = add;
   } else {
      heap_priority pr(prio_queue.min_value());

      prio = pr.float_priority + add;
   }
#ifdef TASK_STEALING
   thread_intrusive_node *tn((thread_intrusive_node*)n);
   tn->lock();
   threads_prio *other((threads_prio*)tn->get_owner());
   if(other == this) {
      do_set_node_priority(n, prio);
   } else {
      // do nothing XXX
   }

   tn->unlock();
#else
   do_set_node_priority(n, prio);
#endif
}

void
threads_prio::add_node_priority_other(node *n, const double priority)
{
#if 0
   // this is called by the other scheduler!
   priority_add_item item;
   item.typ = ADD_PRIORITY;
   item.val = priority;
   item.target = n;
   priority_buffer.push(item);
#else
   (void)n;
   (void)priority;
#endif
}

void
threads_prio::set_node_priority_other(node *n, const double priority)
{
   // this is called by the other scheduler!
#if 0
   priority_add_item item;
   item.typ = SET_PRIORITY;
   item.val = priority;
   item.target = n;
   priority_buffer.push(item);
#else
   (void)n;
   (void)priority;
#endif
}

void
threads_prio::add_node_priority(node *n, const double priority)
{
	thread_intrusive_node *tn((thread_intrusive_node*)n);

#ifdef TASK_STEALING
   tn->lock();
   threads_prio *other((threads_prio*)tn->get_owner());
   if(other == this) {
      const double old_prio(tn->get_float_priority_level());
      do_set_node_priority(n, old_prio + priority);
   } else {
      other->add_node_priority_other(n, priority);
   }
   tn->unlock();
#else
	const double old_prio(tn->get_float_priority_level());

	set_node_priority(n, old_prio + priority);
#endif
}

void
threads_prio::set_node_priority(node *n, const double priority)
{
   thread_intrusive_node *tn((thread_intrusive_node*)n);
#ifdef TASK_STEALING
   tn->lock();
   threads_prio *other((threads_prio*)tn->get_owner());
   if(other == this) {
      do_set_node_priority(n, priority);
   } else {
      other->set_node_priority_other(n, priority);
   }
   tn->unlock();
#else
   (void)tn;
   do_set_node_priority(n, priority);
#endif
}

void
threads_prio::do_set_node_priority(node *n, const double priority)
{
	thread_intrusive_node *tn((thread_intrusive_node*)n);

#ifdef DEBUG_PRIORITIES
   if(priority > 0)
      tn->has_been_prioritized = true;
#endif

   if(current_node == tn) {
      //cout << "Was current node\n";
      if(priority == 0)
         taken_from_priority_queue = false;
      else
         taken_from_priority_queue = true;
      tn->set_float_priority_level(priority);
      return;
   }

   //cout << "================ Set one\n";

	assert(tn->get_owner() == this);
	
	if(tn->in_queue()) {
#ifdef PROFILE_QUEUE
      if(priority > 0)
         prio_immediate++;
#endif
		if(priority_queue::in_queue(tn)) {
         // node is in the priority queue
         if(tn->get_float_priority_level() != priority) {
            if(priority == 0.0) {
               tn->set_float_priority_level(0.0);
               //cout << "Remove from frio put into main\n";
               prio_queue.remove(tn);
               queue_nodes.push_tail(tn);
            } else {
               // priority > 0
               assert(priority > 0.0);
               // we check if new priority is bigger than the current priority
               bool must_change(false);

               switch(priority_type) {
                  case HEAP_FLOAT_DESC: must_change = tn->get_float_priority_level() < priority; break;
                  case HEAP_FLOAT_ASC: must_change = tn->get_float_priority_level() > priority; break;
                  default: assert(false);
               }

               if(must_change) {
                  heap_priority pr;
                  pr.float_priority = priority;
                  tn->set_float_priority_level(priority);
                  assert(tn->in_queue());
                  //cout << "Moving priority\n";
                  prio_queue.move_node(tn, pr);
               }
            }
			} else {
            //cout << "Priority was the same 1 = 1\n";
         }
		} else {
         // node is in the normal queue
         if(priority > 0) {
            tn->set_float_priority_level(priority);
#ifdef DEBUG_PRIORITIES
            //cout << "Add node " << tn->get_id() << " with priority " << priority << endl;
#endif
            if(node_queue::in_queue(tn)) {
               queue_nodes.remove(tn);
            }
            //cout << "Remove from main put into prio\n";
            assert(!priority_queue::in_queue(tn));
            add_to_priority_queue(tn);
         }
		}
		assert(tn->in_queue());
	} else {
#ifdef PROFILE_QUEUE
         prio_marked++;
#endif
      //cout << "Just define priority\n";
      tn->set_float_priority_level(priority);
	}
	
#ifdef PROFILE_QUEUE
	prio_count++;
#endif
   //cout << "============\n";
}

void
threads_prio::init(const size_t)
{
   prio_db = state.all->DATABASE;

   // normal priorities
   switch(state.all->PROGRAM->get_priority_type()) {
      case FIELD_FLOAT:
         if(state.all->PROGRAM->is_priority_desc())
            priority_type = HEAP_FLOAT_DESC;
         else
            priority_type = HEAP_FLOAT_ASC;
         break;
      case FIELD_INT:
         if(state.all->PROGRAM->is_priority_desc())
            priority_type = HEAP_INT_DESC;
         else
            priority_type = HEAP_INT_ASC;
         break;
      default: assert(false);
   }

   prio_queue.set_type(priority_type);

   database::map_nodes::iterator it(state.all->DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state.all->DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      thread_intrusive_node *cur_node((thread_intrusive_node*)it->second);
      heap_priority initial(state.all->PROGRAM->get_initial_priority());
      
      cur_node->set_priority_level(initial);

      init_node(cur_node);

      assert(cur_node->get_owner() == this);
      assert(cur_node->in_queue());
      assert(cur_node->has_work());
   }
   
   threads_synchronize();
}

void
threads_prio::write_slice(statistics::slice& sl) const
{
#ifdef INSTRUMENTATION
   threads_sched::write_slice(sl);
   sl.priority_queue = prio_queue.size();
#else
   (void)sl;
#endif
}

threads_prio::threads_prio(const vm::process_id _id, vm::all *all):
   threads_sched(_id, all)
{
}

threads_prio::~threads_prio(void)
{
}
   
}
