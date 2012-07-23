#include <iostream>
#include <boost/thread/barrier.hpp>

#include "thread/static.hpp"
#include "db/database.hpp"
#include "db/tuple.hpp"
#include "process/remote.hpp"
#include "sched/thread/assert.hpp"
#include "vm/state.hpp"
#include "sched/common.hpp"

using namespace boost;
using namespace std;
using namespace process;
using namespace vm;
using namespace db;
using namespace utils;

#ifdef USE_SIMULATOR
typedef boost::barrier* barrier_ptr;
static barrier_ptr *barriers;
static boost::mutex print_mtx;
using boost::mutex;
static volatile bool all_done;
#endif

namespace sched
{

#ifdef USE_SIMULATOR
void
static_local::get_pending_facts(const quantum_t quantum)
{
	while(!pending_facts.empty()) {
		const quantum_t min((quantum_t)pending_facts.min_value());
		
		if(min <= quantum) {
			{
				mutex::scoped_lock l(print_mtx);
				cout << "New fact with time " << min << endl;
			}
			work w(pending_facts.pop());
			new_agg(w);
		} else
			return;
	}
}
#endif

void
static_local::assert_end(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   assert_static_nodes_end(id);
#ifdef USE_SIMULATOR
	assert(pending_facts.empty());
#endif
}

void
static_local::assert_end_iteration(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   assert_static_nodes_end_iteration(id);
#ifdef USE_SIMULATOR
	assert(pending_facts.empty());
#endif
}

void
static_local::new_agg(work& new_work)
{
   thread_node *to(dynamic_cast<thread_node*>(new_work.get_node()));
   
   assert_thread_push_work();
   
   node_work node_new_work(new_work);
   to->add_work(node_new_work);
   
   if(!to->in_queue()) {
      add_to_queue(to);
      to->set_in_queue(true);
   }
}

void
static_local::new_work(const node *, work& new_work)
{
   thread_node *to(dynamic_cast<thread_node*>(new_work.get_node()));
   
   assert_thread_push_work();
   
   node_work node_new_work(new_work);
   
   to->add_work(node_new_work);
   
   if(!to->in_queue()) {
      spinlock::scoped_lock l(to->spin);
      if(!to->in_queue()) {
         add_to_queue(to);
         to->set_in_queue(true);
      }
      // no need to put owner active, since we own this node
      // new_work was called for init or for self generation of
      // tuples (SEND a TO a)
      // the lock is needed in order to make sure
      // the node is not put multiple times on the queue
   }
   
   assert(to->in_queue());
}

void
static_local::new_work_other(sched::base *, work& new_work)
{
   assert(is_active());
   
   thread_node *tnode(dynamic_cast<thread_node*>(new_work.get_node()));
   
   assert(tnode->get_owner() != NULL);
   
   assert_thread_push_work();
 
#ifdef USE_SIMULATOR
	static_local *owner(dynamic_cast<static_local*>(tnode->get_owner()));
	
	cout << "Thread " << id << " sends message to " << owner->id << " at quantum " << current_quantum << endl;
	state::socket->send_send_message(id, owner->id, new_work.get_tuple()->get_tuple()->get_storage_size(), current_quantum);
	owner->pending_facts.insert(new_work, (int)current_quantum);
	return;
#endif
  
   node_work node_new_work(new_work);
   tnode->add_work(node_new_work);
   
	spinlock::scoped_lock l(tnode->spin);
   if(!tnode->in_queue() && tnode->has_work()) {
		static_local *owner(dynamic_cast<static_local*>(tnode->get_owner()));
		
		tnode->set_in_queue(true);
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
   
#ifdef INSTRUMENTATION
   sent_facts++;
#endif
}

void
static_local::new_work_remote(remote *, const node::node_id, message *)
{
   assert(false);
}

void
static_local::generate_aggs(void)
{
   iterate_static_nodes(id);
}

bool
static_local::busy_wait(void)
{
   ins_idle;

#ifdef USE_SIMULATOR
	cout << "Thread " << id << " is now idle" << endl;
	set_inactive();
	state::socket->send_idle(id);
	while(true) {
		if(id == 0)
			receive_from_simulator();
		if(all_done) {
			printf("DONE\n");
			return false;
		}
		if(please_continue) {
			please_continue = false;
			get_pending_facts(current_quantum);
			assert(start_processing);
			break;
		}
		sleep(1);
	}
#else

   while(!has_work()) {
      BUSY_LOOP_MAKE_INACTIVE()
      BUSY_LOOP_CHECK_TERMINATION_THREADS()
   }
#endif
   
   // since queue pushing and state setting are done in
   // different exclusive regions, this may be needed
   set_active_if_inactive();
   ins_active;
   assert(is_active());
   assert(has_work());
   
   return true;
}

bool
static_local::terminate_iteration(void)
{
   START_ROUND();
   
   if(has_work())
      set_active();
   
   END_ROUND(
      more_work = num_active() > 0;
   );
}

void
static_local::finish_work(const work& work)
{
   base::finish_work(work);
   
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(current_node->get_owner() == this);
}

bool
static_local::check_if_current_useless(void)
{
   if(!current_node->has_work()) {
      spinlock::scoped_lock lock(current_node->spin);
      
      if(!current_node->has_work()) {
         current_node->set_in_queue(false);
         assert(!current_node->in_queue());
         current_node = NULL;
         return true;
      }
   }
   
   assert(current_node->has_work());
   return false;
}

bool
static_local::set_next_node(void)
{
   if(current_node != NULL)
      check_if_current_useless();
   
   while (current_node == NULL) {   
      if(!has_work()) {
         if(!busy_wait())
            return false;
      }
      
      assert(has_work());
      
      current_node = queue_nodes.pop();
      
      assert(current_node->in_queue());
      assert(current_node != NULL);
      
      check_if_current_useless();

   }
   
   ins_active;

#ifdef USE_SIMULATOR
	if(start_processing) {
		left_processing = current_node->size_work();
		previous_quantum = current_quantum;
		{
			mutex::scoped_lock l(print_mtx);
			cout << "Thread " << id << " processing " << left_processing << " facts" << endl;
		}
		start_processing = false;
	}
#endif
   
   assert(current_node != NULL);
   
   return true;
}

#ifdef USE_SIMULATOR
void
static_local::receive_from_simulator(void)
{
	while(true) {
		sim::message_buf buf(state::socket->receive());
		
		switch(SIM_ARG(buf, 0)) {
			case SIM_MESSAGE_UNLOCK:
				{
					const size_t thread(SIM_ARG(buf, 1));
					if(thread == 0) {
						cout << "Thread 0 stopped" << endl;
						return;
					}
					cout << "Unblocking thread " << thread << endl;
					barriers[thread]->wait();
				}
				break;
			case SIM_MESSAGE_TERMINATE:
				{
					all_done = true;
					return;
				}
				break;
			case SIM_MESSAGE_RESTART_THREAD:
				{
					const size_t thread(SIM_ARG(buf, 1));

					if(thread == id) {
						cout << "Thread 0 becomes alive" << endl;
						please_continue = true;
						return;
					} else {
						static_local *other((static_local*)ALL_THREADS[thread]);

						cout << "Thread " << thread << " will now become alive" << endl;

						other->previous_quantum = SIM_ARG(buf, 2);
						other->current_quantum = other->previous_quantum;

						other->please_continue = true;
					}
				}
				break;
			default: cout << "Don't know message type!" << endl;
		}
	}
}
#endif

bool
static_local::get_work(work& new_work)
{ 
#ifdef USE_SIMULATOR
	if(!start_processing && left_processing == 0) {
		// wait for simulator
		{
			mutex::scoped_lock l(print_mtx);
			cout << "Thread " << id << " work lasted for " << current_quantum - previous_quantum << " quanta (total: " << current_quantum << ")" << endl;
			state::socket->send_computation_lock(id, current_quantum - previous_quantum);
			cout << "Thread " << id << " waiting for simulator..." << endl;
		}
		if(id == 0) {
			receive_from_simulator();
			if(all_done)
				return false;
		} else {
			barriers[id]->wait();
		}
		
		{
			mutex::scoped_lock l(print_mtx);
			cout << "Thread " << id << " unlocked" << endl;
		}
		
		get_pending_facts(current_quantum);
		// simulator has responded
		start_processing = true;
	}
#endif

   if(!set_next_node())
      return false;

#ifdef USE_SIMULATOR
	if(!start_processing) {
		left_processing--;
	}
#endif
      
   set_active_if_inactive();
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(current_node->has_work());
   
   node_work unit(current_node->get_work());
   
   new_work.copy_from_node(current_node, unit);

#ifdef USE_SIMULATOR
	current_quantum += state::PROGRAM->get_predicate_quantum(unit.get_tuple()->get_predicate_id());
#endif

   assert(new_work.get_node() == current_node);
   
   assert_thread_pop_work();
   
   return true;
}

void
static_local::end(void)
{
}

void
static_local::init(const size_t)
{
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      thread_node *cur_node((thread_node*)it->second);
      cur_node->set_owner(this);
      
      init_node(cur_node);
      
      assert(cur_node->get_owner() == this);
      assert(cur_node->in_queue());
      assert(cur_node->has_work());
   }
   
   threads_synchronize();
}

static_local*
static_local::find_scheduler(const node *n)
{
   return (static_local*)ALL_THREADS[remote::self->find_proc_owner(n->get_id())];
}

void
static_local::write_slice(statistics::slice& sl) const
{
#ifdef INSTRUMENTATION
   base::write_slice(sl);
   sl.work_queue = queue_nodes.size();
#else
   (void)sl;
#endif
}

static_local::static_local(const vm::process_id _id):
   base(_id),
   current_node(NULL)
#ifdef USE_SIMULATOR
	, current_quantum(0),
	start_processing(true),
	left_processing(0),
	please_continue(false)
#endif
{
}

static_local::~static_local(void)
{
}
   
vector<sched::base*>&
static_local::start(const size_t num_threads)
{
   init_barriers(num_threads);
   
   for(process_id i(0); i < num_threads; ++i)
      add_thread(new static_local(i));

#ifdef USE_SIMULATOR
	barriers = new barrier_ptr[num_threads];
	for(size_t i(0); i < num_threads; ++i) {
		barriers[i] = new boost::barrier(2);
	}
	all_done = false;
#endif
      
   return ALL_THREADS;
}
   
}
