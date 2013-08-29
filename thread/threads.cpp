#include <iostream>
#include <boost/thread/barrier.hpp>

#include "thread/threads.hpp"
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

namespace sched
{

void
threads_sched::assert_end(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   assert_static_nodes_end(id, state.all);
}

void
threads_sched::assert_end_iteration(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   assert_static_nodes_end_iteration(id, state.all);
}

void
threads_sched::new_agg(work& new_work)
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
threads_sched::new_work(const node *, work& new_work)
{
   thread_intrusive_node *to(dynamic_cast<thread_intrusive_node*>(new_work.get_node()));
   
   assert_thread_push_work();

   to->lock();
   
   to->add_work(new_work.get_tuple());
   //cout << id << " Add to queue node " << to->get_id() << endl;
   
   if(!to->in_queue()) {
      to->set_in_queue(true);
      add_to_queue(to);
   }

   assert(to->in_queue());
   to->unlock();
}

void
threads_sched::new_work_other(sched::base *, work& new_work)
{
   assert(is_active());
   
   thread_node *tnode(dynamic_cast<thread_node*>(new_work.get_node()));

   tnode->lock();
   
   threads_sched *owner(dynamic_cast<threads_sched*>(tnode->get_owner()));

   tnode->add_work(new_work.get_tuple());

   if(owner == this) {
      if(!tnode->in_queue()) {
         tnode->set_in_queue(true);
         add_to_queue((thread_intrusive_node*)tnode);
      }
   } else {
      if(!tnode->in_queue()) {
         tnode->set_in_queue(true);
         owner->add_to_queue((thread_intrusive_node*)tnode);
      }

      spinlock::scoped_lock l2(owner->lock);
      
      if(owner->is_inactive() && owner->has_work())
      {
         owner->set_active();
         assert(owner->is_active());
      }
#ifdef INSTRUMENTATION
      sent_facts++;
#endif
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
void
threads_sched::go_steal_nodes(void)
{
   if(state.all->NUM_THREADS == 1)
      return;

   size_t tid(rand(state.all->NUM_THREADS));

   threads_sched *target((threads_sched*)state.all->ALL_THREADS[tid]);

   if(target == this) {
      return;
   }

   if(!target->is_active())
      return;

   thread_intrusive_node *node(NULL);
   size_t size = 1;
   while(size > 0) {
      if(target->number_of_nodes() < 3)
         return;

      node = target->steal_node();

      if(node != NULL) {
         node->lock();
         check_stolen_node(node);
         node->set_owner(this);
         node->set_in_queue(false);
         add_to_queue(node);
         node->set_in_queue(true);
         node->unlock();
#ifdef INSTRUMENTATION
         stolen_total++;
#endif
      }
      size--;
   }
}

thread_intrusive_node*
threads_sched::steal_node(void)
{
   thread_intrusive_node *node(NULL);
   queue_nodes.pop(node);
   return node;
}

size_t
threads_sched::number_of_nodes(void) const
{
   return queue_nodes.size();
}
#endif

void
threads_sched::generate_aggs(void)
{
   iterate_static_nodes(id);
}

bool
threads_sched::busy_wait(void)
{
#ifdef TASK_STEALING
   uint64_t count(0);
#endif
   ins_idle;
   
   while(!has_work()) {
#ifdef TASK_STEALING
      if(!state.all->PROGRAM->is_static_priority()) {
         count++;
         if(count > 1) {
            go_steal_nodes();
            count = 0;
         }
      }
#endif
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

void
threads_sched::finish_work(db::node *no)
{
   base::finish_work(no);
   
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(current_node->get_owner() == this);
}

bool
threads_sched::check_if_current_useless(void)
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
threads_sched::set_next_node(void)
{
   if(current_node != NULL)
      check_if_current_useless();
   
   while (current_node == NULL) {   
      if(!has_work()) {
         if(!busy_wait())
            return false;
      }

      if(!queue_nodes.pop(current_node))
         continue;
      
      assert(current_node->in_queue());
      assert(current_node != NULL);
      
      check_if_current_useless();
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
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(current_node->has_work());
   
   return current_node;
}

void
threads_sched::end(void)
{
}

void
threads_sched::init(const size_t)
{
   const node::node_id first_node(remote::self->find_first_node(id));
   const node::node_id last_node(remote::self->find_last_node(id));

   database::map_nodes::iterator it(state.all->DATABASE->get_node_iterator(first_node));
   database::map_nodes::iterator end(state.all->DATABASE->get_node_iterator(last_node));
   
   for(; it != end; ++it)
   {
      thread_node *cur_node((thread_node*)it->second);
      
      init_node(cur_node);
      
      assert(cur_node->in_queue());
      assert(cur_node->has_work());
   }
   
   threads_synchronize();
}

void
threads_sched::gather_next_tuples(db::node *node, simple_tuple_list& ls)
{
	thread_intrusive_node *no((thread_intrusive_node*)node);
   no->queue.top_list(ls);
}

threads_sched*
threads_sched::find_scheduler(const node *n)
{
   return (threads_sched*)((thread_intrusive_node*)n)->get_owner();
}

void
threads_sched::write_slice(statistics::slice& sl) const
{
#ifdef INSTRUMENTATION
   base::write_slice(sl);
   sl.work_queue = queue_nodes.size();
#ifdef TASK_STEALING
   sl.stolen_nodes = stolen_total;
   sl.steal_requests = steal_requests;
   stolen_total = 0;
   steal_requests = 0;
#endif
#else
   (void)sl;
#endif
}

threads_sched::threads_sched(const vm::process_id _id, vm::all *all):
   base(_id, all),
   current_node(NULL)
#if defined(TASK_STEALING) && defined(INSTRUMENTATION)
   , stolen_total(0), steal_requests(0)
#endif
{
}

threads_sched::~threads_sched(void)
{
}
   
}
