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
	static_local::assert_end();
}

void
threads_prio::assert_end_iteration(void) const
{
	static_local::assert_end_iteration();
   assert(priority_buffer.empty());
}

#ifdef TASK_STEALING
void
threads_prio::check_stolen_nodes(void)
{
   while(!stolen_nodes_buffer.empty()) {
      thread_intrusive_node *n(stolen_nodes_buffer.pop());

      assert(n->in_queue());

#ifdef INSTRUMENTATION
      stolen_total++;
#endif
			
		// nodes may have been added
		if(priority_queue::in_queue(n))
			continue;
		if(node_queue::in_queue(n))
			continue;

      assert(n->moving_around);

		if(n->has_priority_level()) {
         assert(!priority_queue::in_queue(n));
			prio_queue.insert(n, n->get_priority_level());
		} else {
      	queue_nodes.push_tail(n);
		}
      n->moving_around = false;
		assert(n->in_queue());
   }
}

void
threads_prio::answer_steal_requests(void)
{
   bool flag(true);

   while(!steal_request_buffer.empty() && (!queue_nodes.empty() || !prio_queue.empty())) {
      threads_prio *target((threads_prio*)steal_request_buffer.pop());
      thread_intrusive_node *node(NULL);

      size_t size(queue_nodes.size() + prio_queue.size());
      const size_t frac((int)((double)size * (double)state.all->TASK_STEALING_FACTOR));
      size = max(min(size, (size_t)4), frac);

      while(size > 0 && (!queue_nodes.empty() || !prio_queue.empty())) {
         node = NULL;

         if(flag) {
            if(!queue_nodes.empty())
               queue_nodes.pop(node);
            else if(!prio_queue.empty())
               node = prio_queue.pop();
            flag = false;
         } else {
            if(!prio_queue.empty())
               node = prio_queue.pop();
            else if(!queue_nodes.empty())
               queue_nodes.pop(node);
            flag = true;
         }
		
         if(node == NULL)
            continue;

         assert(node != NULL);
         assert(node != current_node);
         assert(node->get_owner() == this);
         assert(node->in_queue());

         node->moving_around = true;
         node->set_owner(target);
         target->stolen_nodes_buffer.push(node);
         size--;
      }

      spinlock::scoped_lock l(target->lock);
   
      if(target->is_inactive() && target->has_work())
      {
         target->set_active();
      }
   }
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

void
threads_prio::new_agg(work& new_work)
{
   thread_intrusive_node *to(dynamic_cast<thread_intrusive_node*>(new_work.get_node()));
   
   assert_thread_push_work();
   
   to->add_work(new_work.get_tuple());
   
   if(!to->in_queue()) {
      to->set_in_queue(true);
		add_to_queue(to);
   }
}

void
threads_prio::new_work(const node *, work& new_work)
{
   thread_intrusive_node *to(dynamic_cast<thread_intrusive_node*>(new_work.get_node()));
   
   assert_thread_push_work();
   
	db::simple_tuple *stpl(new_work.get_tuple());
	const vm::predicate *pred(stpl->get_predicate());
	
   to->add_work(new_work.get_tuple());
   if(!to->in_queue()) {
      to->set_in_queue(true);
      add_to_queue(to);
   }   

	assert(to->in_queue());
}

void
threads_prio::new_work_other(sched::base *, work& new_work)
{
   assert(is_active());
   
   thread_intrusive_node *tnode(dynamic_cast<thread_intrusive_node*>(new_work.get_node()));
   
   assert_thread_push_work();
   
   node_work node_new_work(new_work);
	db::simple_tuple *stpl(node_new_work.get_tuple());
	const vm::predicate *pred(stpl->get_predicate());
	
   threads_prio *owner(dynamic_cast<threads_prio*>(tnode->get_owner()));

   owner->buffer.push(new_work);

   if(this != owner) {
      spinlock::scoped_lock l2(owner->lock);
      
      if(owner->is_inactive() && owner->has_work())
      {
         owner->set_active();
         assert(owner->is_active());
      }
   } else {
      assert(is_active());
   }
   
#ifdef INSTRUMENTATION
   sent_facts++;
#endif
}

#ifdef COMPILE_MPI
void
threads_prio::new_work_remote(remote *, const node::node_id, message *)
{
   assert(false);
}
#endif

void
threads_prio::generate_aggs(void)
{
   iterate_static_nodes(id);
}

bool
threads_prio::busy_wait(void)
{
#ifdef TASK_STEALING
   ins_sched;
   make_steal_request();
   size_t count(0);
#endif
   
   ins_idle;

   while(!has_work()) {
#ifdef TASK_STEALING
      check_stolen_nodes();
      ++count;
      if(count == STEALING_ROUND_MAX) {
         ins_sched;
         make_steal_request();
         ins_idle;
         count = 0;
      }
#endif
      retrieve_tuples();
      check_priority_buffer();
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
threads_prio::terminate_iteration(void)
{
   START_ROUND();
   
   if(has_work())
      set_active();
   
   END_ROUND(
      more_work = num_active() > 0;
   );
}

void
threads_prio::finish_work(db::node *no)
{
   base::finish_work(no);
   
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(current_node->get_owner() == this);
}

bool
threads_prio::check_if_current_useless(void)
{
	assert(current_node->in_queue());
	
	if(!prio_queue.empty() && !current_node->has_work())
   {
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
      current_node = prio_queue.pop();
      //cout << "Prio: " << current_node->get_float_priority_level() << endl;
      taken_from_priority_queue = true;
      return false;
	} else if(!current_node->has_work()) {
      
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
      current_node = NULL;
      return true;
   }
   
   assert(current_node->has_work());
   return false;
}

//#define USE_DELTA_ADJUST
#define ROUND 1000

#ifdef ROUND
static size_t next_round(ROUND);
#endif

bool
threads_prio::set_next_node(void)
{
#ifdef USE_DELTA_ADJUST
   if(id == 0) {
      next_round--;
      if(next_round == 0) {
         next_round = ROUND;
         if(prio_queue.empty()) {
            const float_val cur(state::get_const_float(1));
            if(cur > 0.5) {
               const float_val up(cur / 2.0);
               //cout << "Current " << cur << " new " << up << endl;
               state::set_const_float(1, up);
            }
         }
      }
   }
#endif

   if(current_node != NULL)
      check_if_current_useless();
   
   while (current_node == NULL) {   
#ifdef TASK_STEALING
      check_stolen_nodes();
#endif
      retrieve_tuples();
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

#ifdef TASK_STEALING
   answer_steal_requests();
#endif
      
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
   thread_intrusive_node *tn((thread_intrusive_node*)n);
   double prio(0.0);
   static const double add = 100.0;
   if(prio_queue.empty()) {
      prio = add;
   } else {
      heap_priority pr(prio_queue.min_value());

      prio = pr.float_priority + add;
   }

   set_node_priority(n, prio);
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
#endif
}

void
threads_prio::add_node_priority(node *n, const double priority)
{
	thread_intrusive_node *tn((thread_intrusive_node*)n);

	const double old_prio(tn->get_float_priority_level());

	set_node_priority(n, old_prio + priority);
}

void
threads_prio::set_node_priority(node *n, const double priority)
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
	
	if(tn->in_queue() && !tn->moving_around) {
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
            queue_nodes.remove(tn);
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
      
      cur_node->set_priority_level(state.all->PROGRAM->get_initial_priority());

      init_node(cur_node);

      assert(cur_node->get_owner() == this);
      assert(cur_node->in_queue());
      assert(cur_node->has_work());
   }
   
   threads_synchronize();
}

threads_prio*
threads_prio::find_scheduler(const node *n)
{
	thread_intrusive_node *tn((thread_intrusive_node*)n);
	
	return (threads_prio*)tn->get_owner();
}

simple_tuple_vector
threads_prio::gather_active_tuples(db::node *node, const vm::predicate_id pred)
{
	simple_tuple_vector ls;
	thread_intrusive_node *no((thread_intrusive_node*)node);
	predicate *p(state.all->PROGRAM->get_predicate(pred));
	
	typedef thread_node::queue_type fact_queue;
	
   for(fact_queue::const_iterator it(no->queue.begin()), end(no->queue.end()); it != end; ++it) {
      node_work w(*it);
      simple_tuple *stpl(w.get_tuple());
		
			if(stpl->can_be_consumed() && stpl->get_predicate_id() == pred)
				ls.push_back(stpl);
   }
	
	return ls;
}

void
threads_prio::write_slice(statistics::slice& sl) const
{
#ifdef INSTRUMENTATION
   static_local::write_slice(sl);
   sl.priority_queue = prio_queue.size();
#else
   (void)sl;
#endif
}

threads_prio::threads_prio(const vm::process_id _id, vm::all *all):
   static_local(_id, all)
{
}

threads_prio::~threads_prio(void)
{
}
   
}
