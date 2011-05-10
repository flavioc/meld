
#ifndef SCHED_STATIC_HPP
#define SCHED_STATIC_HPP

#include <list>

#include "sched/base.hpp"
#include "db/node.hpp"
#include "sched/queue/safe_queue.hpp"
#include "utils/types.hpp"

namespace sched
{
   
class sstatic: public sched::base
{
protected:
   
   utils::byte _pad1[128];
   
   size_t iteration;
   
   utils::byte _pad2[128];
   
   safe_queue<work_unit> queue_work;
   
   virtual void assert_end_iteration(void) const;
   
   virtual void assert_end(void) const;
   
   virtual bool busy_wait(void) = 0;
   
   virtual void begin_get_work(void) = 0;
   
   virtual void work_found(void) = 0;
   
   virtual void generate_aggs(void);
   
public:
   
   virtual void new_work(db::node *, db::node *, const db::simple_tuple*, const bool is_agg = false);
   virtual void new_work_agg(db::node *, const db::simple_tuple*);
   virtual void new_work_other(sched::base *, db::node *, const db::simple_tuple *) = 0;
   virtual void new_work_remote(process::remote *, const db::node::node_id, process::message *) = 0;
   
   virtual bool get_work(work_unit&);
   virtual void finish_work(const work_unit&) {};
   
   virtual void init(const size_t);
   virtual void end(void);
   virtual bool terminate_iteration(void);
   
   virtual sstatic *find_scheduler(const db::node::node_id) = 0;
   
   static db::node* create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new db::node(id, trans);
   }
   
   explicit sstatic(const vm::process_id);
   
   virtual ~sstatic(void);
};

}

#endif
