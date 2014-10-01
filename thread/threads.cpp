#include <iostream>
#include <boost/thread/barrier.hpp>
#include <climits>

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
   
   thread_node *tnode(dynamic_cast<thread_node*>(to));

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
      if(!tnode->in_queue()) {
         tnode->set_in_queue(true);
         add_to_queue((thread_intrusive_node*)tnode);
      }
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
      if(!tnode->in_queue()) {
         tnode->set_in_queue(true);
         owner->add_to_queue((thread_intrusive_node*)tnode);
      }
#ifdef INSTRUMENTATION
      sent_facts_other_thread++;
#endif

      spinlock::scoped_lock l2(owner->lock);
      
      if(owner->is_inactive() && owner->has_work())
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
         check_stolen_node(node);
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
   queue_nodes.pop_tail(node);
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

void
threads_sched::killed_while_active(void)
{
   spinlock::scoped_lock l(lock);
   if(is_active())
      set_inactive();
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
      spinlock::scoped_lock lock(current_node->store.spin);
      
      if(!current_node->unprocessed_facts) {
         current_node->set_in_queue(false);
         assert(!current_node->in_queue());
         current_node = NULL;
         return true;
      }
   }
   
   assert(current_node->unprocessed_facts);
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

      if(!queue_nodes.pop_head(current_node))
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
//   cout << "Take node " << current_node->get_id() << endl;

   set_active_if_inactive();
   ins_active;
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(current_node->unprocessed_facts);
   
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

   database::map_nodes::iterator it(All->DATABASE->get_node_iterator(first_node));
   database::map_nodes::iterator end(All->DATABASE->get_node_iterator(last_node));
   
   for(; it != end; ++it)
   {
      thread_intrusive_node *cur_node(dynamic_cast<thread_intrusive_node*>(init_node(it)));

      cur_node->set_in_queue(true);
      add_to_queue(cur_node);
   }
   
   threads_synchronize();
}

void
threads_sched::write_slice(statistics::slice& sl)
{
#ifdef INSTRUMENTATION
   base::write_slice(sl);
   sl.work_queue = queue_nodes.size();
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
{
}

threads_sched::~threads_sched(void)
{
}
   
}

