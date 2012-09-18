#include <iostream>
#include <boost/thread/barrier.hpp>

#include "thread/static_prio.hpp"
#include "db/database.hpp"
#include "db/tuple.hpp"
#include "process/remote.hpp"
#include "sched/thread/assert.hpp"
#include "vm/state.hpp"
#include "sched/common.hpp"
#include "utils/time.hpp"

using namespace boost;
using namespace std;
using namespace process;
using namespace vm;
using namespace db;
using namespace utils;

//#define DEBUG_PRIORITIES
// #define PROFILE_QUEUE

#ifdef PROFILE_QUEUE
static atomic<size_t> prio_count(0);
static atomic<size_t> prio_immediate(0);
static atomic<size_t> prio_marked(0);
static atomic<size_t> prio_moved_pqueue(0);
static atomic<size_t> prio_add_pqueue(0);
static atomic<size_t> prio_removed_pqueue(0);
static atomic<size_t> prio_nodes_compared(0);
static atomic<size_t> prio_nodes_changed(0);
static execution_time queue_time;
#endif

namespace sched
{

void
static_local_prio::assert_end(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   assert_static_nodes_end(id);
}

void
static_local_prio::assert_end_iteration(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   assert_static_nodes_end_iteration(id);
}

void
static_local_prio::retrieve_prio_tuples(void)
{
	while(!prio_tuples.empty()) {
		work new_work(prio_tuples.pop());
		node_work node_new_work(new_work);
		thread_intrusive_node *to(dynamic_cast<thread_intrusive_node*>(new_work.get_node()));
		
		add_prio_tuple(node_new_work, to, node_new_work.get_tuple());
	}
}

void
static_local_prio::new_agg(work& new_work)
{
   thread_intrusive_node *to(dynamic_cast<thread_intrusive_node*>(new_work.get_node()));
   
   assert_thread_push_work();
   
   node_work node_new_work(new_work);
   to->add_work(node_new_work);
   
   if(!to->in_queue()) {
      add_to_queue(to);
      to->set_in_queue(true);
   }
}

void
static_local_prio::add_prio_tuple(node_work node_new_work, thread_intrusive_node *to, db::simple_tuple *stpl)
{
	const field_num field(state::PROGRAM->get_priority_argument());
	vm::tuple *tpl(stpl->get_tuple());
	const int_val val(tpl->get_int(field));
	const int_val min_before(to->get_min_value());
	
	to->prioritized_tuples.insert(node_new_work, val);

	if(to != current_node) {
		spinlock::scoped_lock l(to->spin);
			
		if(global_prioqueue::in_queue(to)) {
			assert(!node_queue::in_queue(to));
				
			const int_val min_now(to->get_min_value());
				
			assert(min_before != thread_intrusive_node::prio_tuples_queue::INVALID_PRIORITY);
				
			if(min_now != min_before) {
#ifdef PROFILE_QUEUE
				prio_moved_pqueue++;
#endif
				gprio_queue.move_node(to, min_now);
			}
		} else {
			if(node_queue::in_queue(to)) {
#ifdef PROFILE_QUEUE
				prio_removed_pqueue++;
#endif
				queue_nodes.remove(to);
			}
			assert(!node_queue::in_queue(to));
				
#ifdef PROFILE_QUEUE
			prio_add_pqueue++;
#endif
			gprio_queue.insert(to, to->get_min_value());
		}
			
		assert(global_prioqueue::in_queue(to));
		assert(!node_queue::in_queue(to));
			
		if(!to->in_queue())
			to->set_in_queue(true);
	}
}

void
static_local_prio::new_work(const node *, work& new_work)
{
   thread_intrusive_node *to(dynamic_cast<thread_intrusive_node*>(new_work.get_node()));
   
   assert_thread_push_work();
   
   node_work node_new_work(new_work);
	db::simple_tuple *stpl(node_new_work.get_tuple());
	const vm::predicate *pred(stpl->get_predicate());
	
#ifdef PROFILE_QUEUE
	queue_time.start();
#endif
	
	if(pred->is_global_priority()) {
		add_prio_tuple(node_new_work, to, stpl);
	} else {
   	to->add_work(node_new_work);
		if(!to->in_queue()) {
	      spinlock::scoped_lock l(to->spin);
	      if(!to->in_queue()) {
				if(!global_prioqueue::in_queue(to))
					add_to_queue(to);
	         to->set_in_queue(true);
	      }
	      // no need to put owner active, since we own this node
	      // new_work was called for init or for self generation of
	      // tuples (SEND a TO a)
	      // the lock is needed in order to make sure
	      // the node is not put multiple times on the queue
	   }   
   }

	assert(to->in_queue());
#ifdef PROFILE_QUEUE
	queue_time.stop();
#endif
}

void
static_local_prio::new_work_other(sched::base *, work& new_work)
{
   assert(is_active());
   
   thread_intrusive_node *tnode(dynamic_cast<thread_intrusive_node*>(new_work.get_node()));
   
   assert(tnode->get_owner() != NULL);
   
   assert_thread_push_work();
   
   node_work node_new_work(new_work);
	db::simple_tuple *stpl(node_new_work.get_tuple());
	const vm::predicate *pred(stpl->get_predicate());
	
	if(pred->is_global_priority()) {
		static_local_prio *owner(dynamic_cast<static_local_prio*>(tnode->get_owner()));
		
		owner->prio_tuples.push(new_work);
		
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
	} else {
   	tnode->add_work(node_new_work);
   
		spinlock::scoped_lock l(tnode->spin);
	
   	if(!tnode->in_queue() && tnode->has_work()) {
			static_local_prio *owner(dynamic_cast<static_local_prio*>(tnode->get_owner()));
		
			tnode->set_in_queue(true);
			if(!global_prioqueue::in_queue(tnode))
				owner->add_to_queue(tnode);

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
         
      	assert(tnode->in_queue());
   	}
	}
   
#ifdef INSTRUMENTATION
   sent_facts++;
#endif
}

void
static_local_prio::new_work_remote(remote *, const node::node_id, message *)
{
   assert(false);
}

void
static_local_prio::generate_aggs(void)
{
   iterate_static_nodes(id);
}

bool
static_local_prio::busy_wait(void)
{
   ins_idle;
   
   while(!has_work()) {
      BUSY_LOOP_MAKE_INACTIVE()
      BUSY_LOOP_CHECK_TERMINATION_THREADS()
   	retrieve_prio_tuples();
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
static_local_prio::terminate_iteration(void)
{
   START_ROUND();
   
   if(has_work())
      set_active();
   
   END_ROUND(
      more_work = num_active() > 0;
   );
}

void
static_local_prio::finish_work(const work& work)
{
   base::finish_work(work);
   
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(current_node->get_owner() == this);
}

bool
static_local_prio::check_if_current_useless(void)
{
	if(!gprio_queue.empty() && current_node->has_prio_work() && !current_node->has_normal_work()) {
		const int current_min(gprio_queue.min_value());
		const int_val node_min(current_node->get_min_value());
		
#ifdef PROFILE_QUEUE
		prio_nodes_compared++;
#endif
		
		if(current_min < node_min) {
			// put back into priority queue
			gprio_queue.insert(current_node, node_min);
			assert(global_prioqueue::in_queue(current_node));
			current_node = gprio_queue.pop();
#ifdef PROFILE_QUEUE
			prio_nodes_changed++;
#endif
		}
		
		assert(current_node->in_queue());
		assert(current_node->has_work());
		return false;
	}
	
   if(!current_node->has_work()) {
      spinlock::scoped_lock lock(current_node->spin);
      
      if(!current_node->has_work()) {
         if(!node_queue::in_queue(current_node)) {
            current_node->set_in_queue(false);
            assert(!current_node->in_queue());
         }
			current_node = NULL;
         return true;
      }
   }
   
   assert(current_node->has_work());
   return false;
}

bool
static_local_prio::set_next_node(void)
{
   if(current_node != NULL)
      check_if_current_useless();
   
   while (current_node == NULL) {   
      if(!has_work()) {
         if(!busy_wait())
            return false;
      }
      
      assert(has_work());

		retrieve_prio_tuples();
		
		if(!gprio_queue.empty()) {
			current_node = gprio_queue.pop();
			goto loop_check;
		}
#ifdef PRIO_OPT
		if(!prio_queue.empty()) {
			const bool suc(prio_queue.pop(current_node));
			if(!suc)
				continue;
		} else if(!queue_nodes.empty()) {
			const bool suc(queue_nodes.pop(current_node));
			if(!suc)
				continue;
		}
#elif defined(PRIO_INVERSE)
		if(!queue_nodes.empty()) {
			const bool suc(queue_nodes.pop(current_node));
			if(!suc)
				continue;
		} else if(!prio_queue.empty()) {
			const bool suc(prio_queue.pop(current_node));
			if(!suc)
				continue;
		}
#else
		const bool suc(queue_nodes.pop(current_node));
		
		assert(suc);
#endif

loop_check:
      assert(current_node->in_queue());
      assert(current_node != NULL);
      
      check_if_current_useless();
   }
   
   ins_active;
   
   assert(current_node != NULL);
   
   return true;
}

bool
static_local_prio::get_work(work& new_work)
{
	retrieve_prio_tuples();
	
   if(!set_next_node())
      return false;
      
   set_active_if_inactive();
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(current_node->has_work());
   
	node_work unit;
	
	if(current_node->has_normal_work())
		unit = current_node->get_work();
	else
		unit = current_node->prioritized_tuples.pop();
   
   new_work.copy_from_node(current_node, unit);
   
   assert(new_work.get_node() == current_node);
   
   assert_thread_pop_work();
   
   return true;
}

void
static_local_prio::end(void)
{
#ifndef PRIO_NORMAL
#ifdef DEBUG_PRIORITIES
	cout << "prio_immediate: " << prio_immediate << endl;
	cout << "prio_marked: " << prio_marked << endl;
	cout << "prio_count: " << prio_count << endl;
#endif
#ifdef PROFILE_QUEUE
	cout << "Moved nodes in priority queue count: " << prio_moved_pqueue << endl;
	cout << "Removed from normal queue count: " << prio_removed_pqueue << endl;
	cout << "Added to priority queue count: " << prio_add_pqueue << endl;
	cout << "Changed nodes count: " << prio_nodes_changed << endl;
	cout << "Compared nodes count: " << prio_nodes_compared << endl;
#endif
#endif
#ifdef PROFILE_QUEUE
	cout << "Queue time: " << queue_time.milliseconds() << " ms" << endl;
#endif
}

void
static_local_prio::set_node_priority(node *n, const int)
{
#ifdef PRIO_NORMAL
	(void)n;
#elif defined(PRIO_HEAD)
	(void)n;
#elif defined(PRIO_OPT) || defined(PRIO_INVERSE)
	thread_intrusive_node *tn((thread_intrusive_node*)n);
	
	if(tn->in_queue()) {
#ifdef PROFILE_QUEUE
		prio_immediate++;
#endif
		if(!tn->is_in_prioqueue()) {
			queue_nodes.remove(tn);
			prio_queue.push_tail(tn);
			tn->set_is_in_prioqueue(true);
		}
		assert(tn->is_in_prioqueue());
	} else {
#ifdef PROFILE_QUEUE
		prio_marked++;
#endif
		tn->mark_priority();
	}
#endif
	
#ifdef PROFILE_QUEUE
	prio_count++;
#endif
}

void
static_local_prio::init(const size_t)
{
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      thread_intrusive_node *cur_node((thread_intrusive_node*)it->second);
      cur_node->set_owner(this);
      
      init_node(cur_node);
      
      assert(cur_node->get_owner() == this);
      assert(cur_node->in_queue());
      assert(cur_node->has_work());
   }
   
   threads_synchronize();
}

static_local_prio*
static_local_prio::find_scheduler(const node *n)
{
   return (static_local_prio*)ALL_THREADS[remote::self->find_proc_owner(n->get_id())];
}

simple_tuple_list
static_local_prio::gather_active_tuples(db::node *node, const vm::predicate_id pred)
{
	simple_tuple_list ls;
	thread_intrusive_node *no((thread_intrusive_node*)node);
	predicate *p(state::PROGRAM->get_predicate(pred));
	
	typedef thread_node::queue_type fact_queue;
	
	if(p->is_global_priority()) {
		typedef thread_intrusive_node::prio_tuples_queue prio_queue;
		for(prio_queue::const_iterator it(no->prioritized_tuples.begin()),
				end(no->prioritized_tuples.end());
			it != end; ++it)
		{
			node_work w(*it);
			simple_tuple *stpl(w.get_tuple());
		
			if(!stpl->must_be_deleted())
				ls.push_back(stpl);
		}
	} else {
		for(fact_queue::const_iterator it(no->queue.begin()), end(no->queue.end()); it != end; ++it) {
			node_work w(*it);
			simple_tuple *stpl(w.get_tuple());
		
			if(!stpl->must_be_deleted() && stpl->get_predicate_id() == pred)
				ls.push_back(stpl);
		}
	}
	
	return ls;
}

void
static_local_prio::write_slice(statistics::slice& sl) const
{
#ifdef INSTRUMENTATION
   base::write_slice(sl);
   sl.work_queue = queue_nodes.size();
#else
   (void)sl;
#endif
}

static_local_prio::static_local_prio(const vm::process_id _id):
   base(_id),
   current_node(NULL)
{
}

static_local_prio::~static_local_prio(void)
{
}
   
vector<sched::base*>&
static_local_prio::start(const size_t num_threads)
{
   init_barriers(num_threads);
   
   for(process_id i(0); i < num_threads; ++i)
      add_thread(new static_local_prio(i));
      
   return ALL_THREADS;
}
   
}
