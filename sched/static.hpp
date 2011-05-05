
#ifndef SCHED_STATIC_HPP
#define SCHED_STATIC_HPP

#include <list>

#include "sched/base.hpp"
#include "db/node.hpp"
#include "sched/queue.hpp"
#include "utils/types.hpp"

namespace sched
{
   
class sstatic: public sched::base
{
protected:
   
   const vm::process_id id;
   
   utils::byte _pad1[128];
   
   size_t iteration;
   
   utils::byte _pad2[128];
   
   wqueue<work_unit> queue_work;
   
   virtual void assert_end_iteration(void) const;
   
   virtual void assert_end(void) const;
   
   virtual bool busy_wait(void) = 0;
   
   virtual void begin_get_work(void) = 0;
   
   virtual void work_found(void) = 0;
   
   virtual void generate_aggs(void);
   
public:
   
   virtual void new_work(db::node *, const db::simple_tuple*, const bool is_agg = false);
   virtual void new_work_other(sched::base *, db::node *, const db::simple_tuple *) = 0;
   virtual void new_work_remote(process::remote *, const vm::process_id, process::message *) = 0;
   
   virtual bool get_work(work_unit&);
   
   virtual void init(const size_t);
   virtual void end(void);
   virtual bool terminate_iteration(void);
   
   explicit sstatic(const vm::process_id);
   
   virtual ~sstatic(void);
};

}

#endif