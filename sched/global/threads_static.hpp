
#ifndef SCHED_GLOBAL_THREADS_THREADS_HPP
#define SCHED_GLOBAL_THREADS_THREADS_HPP

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <vector>

#include "sched/global/static.hpp"
#include "sched/thread/threaded.hpp"
#include "sched/queue/simple_linear_pqueue.hpp"
#include "vm/defs.hpp"

namespace sched
{
   
class static_global : public sched::sstatic,
                       public sched::threaded
{
private:
   
   utils::byte _pad_threads1[128];
   
   typedef simple_linear_pqueue<work_unit> queue_free_work;
   std::vector<queue_free_work, mem::allocator<queue_free_work> > buffered_work;
   
   bool all_buffers_emptied(void) const;
   void flush_this_queue(queue_free_work&, static_global *);
   void flush_buffered(void);
   
protected:
   
   virtual void assert_end_iteration(void) const;
   virtual void assert_end(void) const;
   
   virtual bool busy_wait(void);
   
   virtual void begin_get_work(void);
   
   virtual void work_found(void);
   
public:
   
   virtual void new_work(db::node *, db::node *, const db::simple_tuple*, const bool is_agg = false);
   virtual void new_work_other(sched::base *, db::node *, const db::simple_tuple *);
   virtual void new_work_remote(process::remote *, const db::node::node_id, process::message *);
   
   virtual void init(const size_t);
   virtual void end(void);
   virtual bool terminate_iteration(void);
   virtual bool get_work(work_unit&);
   
   static_global *find_scheduler(const db::node::node_id);
   
   static std::vector<sched::base*>& start(const size_t num_threads);
   
   explicit static_global(const vm::process_id);
   
   virtual ~static_global(void);
};

}

#endif
