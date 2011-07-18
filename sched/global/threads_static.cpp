
#include <iostream>
#include <boost/thread/barrier.hpp>

#include "sched/global/threads_static.hpp"

#include "vm/state.hpp"
#include "process/machine.hpp"
#include "sched/thread/termination_barrier.hpp"
#include "sched/thread/assert.hpp"
#include "sched/common.hpp"

using namespace boost;
using namespace vm;
using namespace db;
using namespace process;
using namespace std;
using namespace utils;

namespace sched
{

void
static_global::new_work(const node *, work& new_work)
{  
   queue_work.push(new_work, new_work.get_strat_level());
   
   assert((!new_work.force_aggregate() && is_active()) || new_work.force_aggregate());
   assert_thread_push_work();
}

void
static_global::new_work_other(sched::base *scheduler, work& new_work)
{
   assert(scheduler != NULL);
   
   static_global *other((static_global*)scheduler);
   const process_id other_id(other->id);
   
   if(buf.push(other_id, new_work))
      flush_queue(other_id, other);
      
   assert_thread_push_work();
}

void
static_global::new_work_remote(remote *, const node::node_id, message *)
{
   assert(false);
}

void
static_global::assert_end_iteration(void) const
{
   assert(is_inactive());
   assert(buf.empty());
   assert(all_threads_finished());
   assert(!has_work());
   assert_static_nodes_end_iteration(id);
}

void
static_global::assert_end(void) const
{
   assert(is_inactive());
   assert(buf.empty());
   assert(all_threads_finished());
   assert(!has_work());
   assert_static_nodes_end(id);
}

void
static_global::flush_queue(const process_id id, static_global *other)
{
   queue_buffer::queue& q(buf.get_queue(id));
   
   assert(this != other);
   assert(!q.empty());
   
#ifdef INSTRUMENTATION
   sent_facts += q.size();
#endif

   other->queue_work.snap(q);
   
   MAKE_OTHER_ACTIVE(other);
   
   buf.clear_queue(id);
}

void
static_global::flush_buffered(void)
{
   if(buf.empty())
      return;
   
   for(process_id i(0); i < (process_id)state::NUM_THREADS; ++i) {
      if(i != id && !buf.empty(i))
         flush_queue(i, (static_global*)ALL_THREADS[i]);
   }
}
   
bool
static_global::busy_wait(void)
{
   flush_buffered();
   
   while(!has_work()) {
      BUSY_LOOP_MAKE_INACTIVE()
      BUSY_LOOP_CHECK_TERMINATION_THREADS()
   }
   
   set_active_if_inactive();
   
   assert(is_active());
   assert(has_work());
   
   return true;
}

bool
static_global::get_work(work& new_work)
{
   if(!has_work()) {
      if(!busy_wait())
         return false;

      assert(is_active());
      assert(has_work());
   }
   
   new_work = queue_work.pop();
   
   assert_thread_pop_work();
   
   return true;
}

void
static_global::init(const size_t num_threads)
{
   const node::node_id first(remote::self->find_first_node(id));
   const node::node_id final(remote::self->find_last_node(id));
   
   database::map_nodes::iterator it(state::DATABASE->get_node_iterator(first));
   database::map_nodes::iterator end(state::DATABASE->get_node_iterator(final));
   
   for(; it != end; ++it)
      init_node(it->second);
      
   buf.init(num_threads);
      
   assert(is_active());
}

void
static_global::end(void)
{
   assert(is_inactive());
}

void
static_global::generate_aggs(void)
{
   iterate_static_nodes(id);
}

bool
static_global::terminate_iteration(void)
{
   // this is needed since one thread can reach make_active
   // and thus other threads waiting for all_finished will fail
   // to get here
   
   assert_thread_end_iteration();
   threads_synchronize();
   
   assert(buf.empty());
   assert(is_inactive());
   
   generate_aggs();
   
   if(has_work())
      set_active();
      
   assert_thread_iteration(iteration);
   
   // again, needed since we must wait if any thread
   // is set to active in the previous if
   
   threads_synchronize();
   
   const bool ret(!all_threads_finished());
   
   // if we don't synchronize here we risk that other threads get ahead
   // and turn into inactive and we have work to do
   threads_synchronize();
   
   return ret;
}

static_global*
static_global::find_scheduler(const node::node_id id)
{
   return (static_global*)ALL_THREADS[remote::self->find_proc_owner(id)];
}

void
static_global::write_slice(stat::slice& sl) const
{
#ifdef INSTRUMENTATION
   base::write_slice(sl);
   threaded::write_slice(sl);
   sl.work_queue = queue_work.size();
#else
   (void)sl;
#endif
}

static_global::static_global(const process_id _id):
   base(_id),
   queue_work(vm::predicate::MAX_STRAT_LEVEL)
{
}

static_global::~static_global(void)
{
}

vector<sched::base*>&
static_global::start(const size_t num_threads)
{
   init_barriers(num_threads);
   
   for(process_id i(0); i < num_threads; ++i)
      add_thread(new static_global(i));
      
   return ALL_THREADS;
}

}
