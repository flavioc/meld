
#include <iostream>
#include <boost/thread/barrier.hpp>

#include "sched/static_global.hpp"

#include "vm/state.hpp"
#include "process/machine.hpp"
#include "sched/termination_barrier.hpp"

using namespace boost;
using namespace vm;
using namespace db;
using namespace process;
using namespace std;

namespace sched
{
  
bool
static_global::all_buffers_emptied(void) const
{
   for(process_id i(0); i < (process_id)buffered_work.size(); ++i) {
      if(!buffered_work[i].empty())
         return false;
   }
   return true;
}

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
   
   static const size_t WORK_THRESHOLD(20);
   
   static_global *other((static_global*)scheduler);
   work_unit work = {node, stuple, false};
   queue_free_work& q(buffered_work[other->id]);
   
   q.push(work);
   
   if(q.size() > WORK_THRESHOLD)
      flush_this_queue(q, other);
}

void
static_global::new_work_remote(remote *, const vm::process_id, message *)
{
   assert(false);
}

void
static_global::assert_end_iteration(void) const
{
   sstatic::assert_end_iteration();
   assert(is_inactive());
   assert(all_buffers_emptied());
   assert(all_threads_finished());
}

void
static_global::assert_end(void) const
{
   sstatic::assert_end();
   assert(is_inactive());
   assert(all_buffers_emptied());
   assert(all_threads_finished());
}

void
static_global::flush_this_queue(queue_free_work& q, static_global *other)
{
   assert(this != other);
   assert(is_active());
   
   other->queue_work.snap(q);
   
   {
      mutex::scoped_lock l(other->mutex);
      if(other->is_inactive())
         other->set_active();
   }
   
   q.clear();
}

void
static_global::flush_buffered(void)
{
   for(process_id i(0); i < (process_id)buffered_work.size(); ++i) {
      if(i != id) {
         queue_free_work& q(buffered_work[i]);
         if(!q.empty()) {
            assert(is_active());
            flush_this_queue(q, (static_global*)ALL_THREADS[i]);
         }
      }
   }
}
   
bool
static_global::busy_wait(void)
{
   bool turned_inactive(false);
   
   flush_buffered();
   
   while(queue_work.empty()) {
      
      if(!turned_inactive) {
         mutex::scoped_lock l(mutex);
         if(queue_work.empty()) {
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
   
   if(is_inactive()) {
      mutex::scoped_lock l(mutex);
      if(is_inactive())
         set_active();
   }
   
   assert(is_active());
   assert(!queue_work.empty());
   
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
   assert(!queue_work.empty());
}

void
static_global::init(const size_t num_threads)
{
   sstatic::init(num_threads);
   
   // init buffered queues
   buffered_work.resize(num_threads);
   
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
   
   assert(all_buffers_emptied());
   assert(is_inactive());
   
   generate_aggs();
   
   if(!queue_work.empty())
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
   
   return !all_threads_finished();
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
