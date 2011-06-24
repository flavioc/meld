
#ifndef SCHED_GLOBAL_THREADS_THREADS_HPP
#define SCHED_GLOBAL_THREADS_THREADS_HPP

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <vector>

#include "sched/global/static.hpp"
#include "sched/thread/threaded.hpp"
#include "sched/thread/queue_buffer.hpp"
#include "utils/macros.hpp"
#include "vm/defs.hpp"

namespace sched
{
   
class static_global : public sched::sstatic,
                       public sched::threaded
{
private:
   
   DEFINE_PADDING;
   
   queue_buffer buf;
   
   void flush_queue(const vm::process_id, static_global *);
   
protected:
   
   void flush_buffered(void);
   
   virtual void assert_end_iteration(void) const;
   virtual void assert_end(void) const;
   virtual bool busy_wait(void);
   virtual void begin_get_work(void);
   virtual void work_found(void);
   
public:
   
   virtual void new_work(db::node *, db::node *, const db::simple_tuple*, const bool is_agg = false);
   virtual void new_work_other(sched::base *, db::node *, const db::simple_tuple *);
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
   
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
