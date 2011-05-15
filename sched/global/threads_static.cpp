
#include <iostream>
#include <boost/thread/barrier.hpp>

#include "sched/global/threads_static.hpp"

#include "vm/state.hpp"
#include "process/machine.hpp"
#include "sched/thread/termination_barrier.hpp"

using namespace boost;
using namespace vm;
using namespace db;
using namespace process;
using namespace std;

namespace sched
{

void
static_global::new_work(node *from, node *to, const simple_tuple *tpl, const bool is_agg)
{
   assert(to != NULL);
   assert(tpl != NULL);
   sstatic::new_work(from, to, tpl, is_agg);
   assert(is_active());
}

void
static_global::new_work_other(sched::base *scheduler, node *node, const simple_tuple *stuple)
{
   assert(is_active());
   assert(node != NULL);
   assert(stuple != NULL);
   assert(scheduler != NULL);
   
   static_global *other((static_global*)scheduler);
   work_unit work = {node, stuple, false};
   const process_id other_id(other->id);
   
   if(buf.push(other_id, work))
      flush_queue(other_id, other);
}

void
static_global::new_work_remote(remote *, const node::node_id, message *)
{
   assert(false);
}

void
static_global::assert_end_iteration(void) const
{
   sstatic::assert_end_iteration();
   assert(is_inactive());
   assert(buf.empty());
   assert(all_threads_finished());
}

void
static_global::assert_end(void) const
{
   sstatic::assert_end();
   assert(is_inactive());
   assert(buf.empty());
   assert(all_threads_finished());
}

void
static_global::flush_queue(const process_id id, static_global *other)
{
   queue_buffer::queue& q(buf.get_queue(id));
   
   assert(this != other);
   assert(is_active());
   assert(!q.empty());
   
   other->queue_work.snap(q);
   
   if(other->is_inactive()) {
      mutex::scoped_lock l(other->mutex);
      if(other->is_inactive())
         other->set_active();
   }
   
   buf.clear_queue(id);
}

void
static_global::flush_buffered(void)
{
   if(buf.empty())
      return;
      
   assert(is_active());
   
   for(process_id i(0); i < (process_id)state::NUM_THREADS; ++i) {
      if(i != id && !buf.empty(i))
         flush_queue(i, (static_global*)ALL_THREADS[i]);
   }
}
   
bool
static_global::busy_wait(void)
{
   bool turned_inactive(false);
   
   flush_buffered();
   
   while(!has_work()) {
      
      if(!turned_inactive) {
         mutex::scoped_lock l(mutex);
         if(!has_work()) {
            if(is_active()) // may be inactive from the previous iteration
               set_inactive();
            turned_inactive = true;
            if(all_threads_finished())
               return false;
         }
      }
      
      if(turned_inactive && is_inactive() && all_threads_finished()) {
         assert(turned_inactive);
         assert(is_inactive());
         return false;
      }
   }
   
   set_active_if_inactive();
   
   assert(is_active());
   assert(has_work());
   
   return true;
}

bool
static_global::get_work(work_unit& work)
{
   return sstatic::get_work(work);
}

void
static_global::begin_get_work(void)
{
}

void
static_global::work_found(void)
{
   assert(is_active());
   assert(has_work());
}

void
static_global::init(const size_t num_threads)
{
   sstatic::init(num_threads);
   buf.init(num_threads);
      
   assert(is_active());
}

void
static_global::end(void)
{
   sstatic::end();
   assert(is_inactive());
}

bool
static_global::terminate_iteration(void)
{
   // this is needed since one thread can reach make_active
   // and thus other threads waiting for all_finished will fail
   // to get here
   
   threads_synchronize();
   
   sstatic::terminate_iteration();
   
   assert(buf.empty());
   assert(is_inactive());
   
   generate_aggs();
   
   if(has_work())
      set_active();
   
#ifdef ASSERT_THREADS
   static boost::mutex local_mtx;
   static vector<size_t> total;
   
   local_mtx.lock();
   
   total.push_back(iteration);
   
   if(total.size() == state::NUM_THREADS) {
      for(size_t i(0); i < state::NUM_THREADS; ++i) {
         assert(total[i] == iteration);
      }
      
      total.clear();
   }
   
   local_mtx.unlock();
#endif
   
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

static_global::static_global(const process_id _id):
   sstatic(_id)
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
