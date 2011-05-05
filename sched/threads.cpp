
#include <iostream>
#include <boost/thread/barrier.hpp>

#include "sched/threads.hpp"

#include "vm/state.hpp"
#include "process/machine.hpp"

using namespace boost;
using namespace vm;
using namespace db;
using namespace process;
using namespace std;

//#define DEBUG_ACTIVE 1

namespace sched
{
   
static size_t threads_active(0);
static vector<threads_static*> others;
static barrier *thread_barrier(NULL);
 
static inline void
make_thread_active(void)
{
   __sync_fetch_and_add(&threads_active, 1);
}

static inline void
make_thread_inactive(void)
{
   __sync_fetch_and_sub(&threads_active, 1);
}

static inline const bool
are_threads_finished(void)
{
   return threads_active == 0;
}

static inline void
threads_synchronize(void)
{
   thread_barrier->wait();
}
  
bool
threads_static::all_buffers_emptied(void) const
{
   for(process_id i(0); i < (process_id)buffered_work.size(); ++i) {
      if(!buffered_work[i].empty())
         return false;
   }
   return true;
}

void
threads_static::new_work(node *node, const simple_tuple *tpl, const bool is_agg)
{
   sstatic::new_work(node, tpl, is_agg);
   make_active();
}

void
threads_static::new_work_other(sched::base *scheduler, node *node, const simple_tuple *stuple)
{
   static const size_t WORK_THRESHOLD(20);
   
   threads_static *other((threads_static*)scheduler);
   work_unit work = {node, stuple, false};
   wqueue_free<work_unit>& q(buffered_work[other->id]);
   
   q.push(work);
   
   if(q.size() > WORK_THRESHOLD)
      flush_this_queue(q, other);
}

void
threads_static::new_work_remote(remote *, const vm::process_id, message *)
{
   assert(0);
}

void
threads_static::assert_end_iteration(void) const
{
   sstatic::assert_end_iteration();
   assert(process_state == PROCESS_INACTIVE);
   assert(all_buffers_emptied());
   assert(threads_active == 0);
}

void
threads_static::assert_end(void) const
{
   sstatic::assert_end();
   assert(process_state == PROCESS_INACTIVE);
   assert(all_buffers_emptied());
   assert(threads_active == 0);
}

void
threads_static::flush_this_queue(wqueue_free<work_unit>& q, threads_static *other)
{
   assert(this != other);
   
   other->make_active();

   other->queue_work.snap(q);
   
   q.clear();
}

void
threads_static::flush_buffered(void)
{
   for(process_id i(0); i < (process_id)buffered_work.size(); ++i) {
      if(i != id) {
         wqueue_free<work_unit>& q(buffered_work[i]);
         if(!q.empty())
            flush_this_queue(q, others[i]);
      }
   }
}
   
bool
threads_static::busy_wait(void)
{
   static const size_t COUNT_UP_TO(2);
    
   size_t cont(0);
   bool turned_inactive(false);
   
   flush_buffered();
   
   while(queue_work.empty()) {
      
      if(are_threads_finished())
         return false;

      if(cont >= COUNT_UP_TO && !turned_inactive) {
         make_inactive();
         turned_inactive = true;
      } else if(cont < COUNT_UP_TO)
         cont++;
   }
   
   assert(!queue_work.empty());
   
   return true;
}

void
threads_static::make_active(void)
{
   if(process_state == PROCESS_INACTIVE) {
      mutex::scoped_lock l(mutex);
      
      if(process_state == PROCESS_INACTIVE) {
         make_thread_active();
         process_state = PROCESS_ACTIVE;
#ifdef DEBUG_ACTIVE
         cout << "Active " << id << endl;
#endif
      }   
   }
}

void
threads_static::make_inactive(void)
{
   if(process_state == PROCESS_ACTIVE) {
      mutex::scoped_lock l(mutex);

      if(process_state == PROCESS_ACTIVE) {
         make_thread_inactive();
         process_state = PROCESS_INACTIVE;
#ifdef DEBUG_ACTIVE
         cout << "Inactive: " << id << endl;
#endif
      }
   }
}

void
threads_static::begin_get_work(void)
{
}

void
threads_static::work_found(void)
{
   make_active();
}

void
threads_static::init(const size_t num_threads)
{
   sstatic::init(num_threads);
   
   // init buffered queues
   buffered_work.resize(num_threads);
}

void
threads_static::end(void)
{
   sstatic::end();
   cout << "Iterations: " << iteration << endl;
}

bool
threads_static::terminate_iteration(void)
{
   sstatic::terminate_iteration();
   
   threads_synchronize();
   
   generate_aggs();
   
   threads_synchronize();
   
   if(are_threads_finished())
      return false;
   
   threads_synchronize();
   
   return true;
}

threads_static::threads_static(const process_id _id):
   sstatic(),
   id(_id),
   process_state(PROCESS_ACTIVE)
{
}

threads_static::~threads_static(void)
{
}

vector<threads_static*>&
threads_static::start(const size_t num_threads)
{
   thread_barrier = new barrier(num_threads);
   others.resize(num_threads);
   threads_active = num_threads;
   
   for(process_id i(0); i < num_threads; ++i)
      others[i] = new threads_static(i);
      
   return others;
}

}