#include <iostream>
#include <boost/thread/barrier.hpp>

#include "sched/local/threads_single.hpp"
#include "db/database.hpp"
#include "db/tuple.hpp"
#include "process/remote.hpp"
#include "sched/thread/assert.hpp"
#include "vm/state.hpp"
#include "utils/time.hpp"

using namespace boost;
using namespace std;
using namespace process;
using namespace vm;
using namespace db;
using namespace utils;

namespace sched
{
   
safe_queue_multi<thread_node*> threads_single::queue_nodes;

void
threads_single::assert_end(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   
   const node::node_id first(remote::self->find_first_node(id));
   const node::node_id final(remote::self->find_last_node(id));
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(final));

   for(; it != end; ++it) {
      thread_node *node((thread_node*)it->second);
      node->assert_end();
      assert(!node->in_queue());
   }
}

void
threads_single::assert_end_iteration(void) const
{
   assert(!has_work());
   assert(is_inactive());
   assert(all_threads_finished());
   assert_thread_end_iteration();
   
   const node::node_id first(remote::self->find_first_node(id));
   const node::node_id final(remote::self->find_last_node(id));
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(final));

   for(; it != end; ++it) {
      thread_node *node((thread_node*)it->second);
      node->assert_end_iteration();
      assert(!node->in_queue());
   }
}

void
threads_single::new_work(node *from, node *_to, const simple_tuple *tpl, const bool is_agg)
{
   (void)from;
   thread_node *to((thread_node*)_to);
    
   assert(to != NULL);
   assert(tpl != NULL);
   
   assert_thread_push_work();
   
   to->add_work(tpl, is_agg);
   
   if(!to->in_queue() && !to->no_more_work()) {
      spinlock::scoped_lock l(to->spin);
      if(!to->in_queue() && !to->no_more_work()) {
         to->set_in_queue(true);
         add_to_queue(to);
      }
   }
}

void
threads_single::new_work_other(sched::base *scheduler, node *node, const simple_tuple *stuple)
{
   assert(node != NULL);
   assert(stuple != NULL);
   assert(scheduler == NULL);
   
   thread_node *tnode((thread_node*)node);
   
   assert_thread_push_work();
   
   tnode->add_work(stuple, false);
   
   if(!tnode->in_queue() && !tnode->no_more_work()) {
      spinlock::scoped_lock l(tnode->spin);
      if(!tnode->in_queue() && !tnode->no_more_work()) {
         tnode->set_in_queue(true);
         add_to_queue(tnode);
         assert(tnode->in_queue());
      }
   }
}

void
threads_single::new_work_remote(remote *, const node::node_id, message *)
{
   assert(false);
}

void
threads_single::generate_aggs(void)
{
   const node::node_id first(remote::self->find_first_node(id));
   const node::node_id final(remote::self->find_last_node(id));
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(final));

   for(; it != end; ++it)
      node_iteration(it->second);
}

bool
threads_single::busy_wait(void)
{
   while(!has_work()) {
      BUSY_LOOP_MAKE_INACTIVE()
      BUSY_LOOP_CHECK_TERMINATION_THREADS()
   }
   
   // since queue pushing and state setting are done in
   // different exclusive regions, this may be needed
   set_active_if_inactive();
   
   assert(is_active());
   
   return true;
}

bool
threads_single::terminate_iteration(void)
{
   // this is needed since one thread can reach set_active
   // and thus other threads waiting for all_finished will fail
   // to get here
   
   assert_thread_end_iteration();
   
   threads_synchronize();

   assert(is_inactive());

   generate_aggs();

   assert_thread_iteration(iteration);

   // again, needed since we must wait if any thread
   // is set to active in the previous if
   threads_synchronize();

   const bool ret(has_work());
   
   if(ret)
      set_active();
   
   threads_synchronize();
   
   return ret;
}

void
threads_single::finish_work(const work_unit& work)
{
   assert(current_node != NULL);
   assert(current_node->in_queue());
}

bool
threads_single::check_if_current_useless(void)
{
   if(current_node->no_more_work()) {
      spinlock::scoped_lock l(current_node->spin);
      
      if(current_node->no_more_work()) {
         current_node->set_in_queue(false);
         assert(!current_node->in_queue());
         current_node = NULL;
         return true;
      }
   }
   
   return false;
}

bool
threads_single::set_next_node(void)
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
   
   assert(current_node != NULL);
   return true;
}

bool
threads_single::get_work(work_unit& work)
{  
   if(!set_next_node())
      return false;
      
   assert(current_node != NULL);
   assert(current_node->in_queue());
   assert(!current_node->no_more_work());
   
   node_work_unit unit(current_node->get_work());
   
   work.work_tpl = unit.work_tpl;
   work.agg = unit.agg;
   work.work_node = current_node;
   
   assert(work.work_node == current_node);
   
   assert_thread_pop_work();
   
   return true;
}

void
threads_single::init(const size_t num_threads)
{
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(remote::self->find_first_node(id)));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(remote::self->find_last_node(id)));
   
   for(; it != end; ++it)
   {
      thread_node *cur_node((thread_node*)it->second);
      
      init_node(cur_node);
      
      assert(cur_node->in_queue());
      assert(!cur_node->no_more_work());
   }
   
   threads_synchronize();
}

threads_single*
threads_single::find_scheduler(const node::node_id id)
{
   return NULL;
}

threads_single::threads_single(const vm::process_id _id):
   base(_id),
   current_node(NULL)
{
}

threads_single::~threads_single(void)
{
}
   
vector<sched::base*>&
threads_single::start(const size_t num_threads)
{
   init_barriers(num_threads);
   
   for(process_id i(0); i < num_threads; ++i)
      add_thread(new threads_single(i));
      
   return ALL_THREADS;
}
   
}