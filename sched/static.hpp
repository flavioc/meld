
#ifndef SCHED_STATIC_HPP
#define SCHED_STATIC_HPP

#include <list>

#include "sched/base.hpp"
#include "db/node.hpp"
#include "utils/interval.hpp"
#include "process/queue.hpp"
#include "utils/types.hpp"

namespace sched
{
   
class sstatic: public sched::base
{
private:
   
   typedef std::list<db::node*> list_nodes;
   
   list_nodes nodes;
   
   utils::interval<db::node::node_id> *nodes_interval;
   
   utils::byte _pad1[128];
   
protected:
   
   size_t iteration;
   
   utils::byte _pad2[128];
   
   process::wqueue<work_unit> queue_work;
   
   virtual void assert_end_iteration(void) const;
   
   virtual void assert_end(void) const;
   
   virtual bool busy_wait(void) = 0;
   
   virtual void begin_get_work(void) = 0;
   
   virtual void work_found(void) = 0;
   
   virtual void generate_aggs(void);
   
public:
   
   virtual void add_node(db::node *);
   
   virtual void new_work(db::node *, const db::simple_tuple*, const bool is_agg = false);
   virtual void new_work_other(sched::base *, db::node *, const db::simple_tuple *) = 0;
   virtual void new_work_remote(process::remote *, const vm::process_id, process::message *) = 0;
   
   virtual bool get_work(work_unit&);
   
   virtual void init(const size_t);
   virtual void end(void);
   virtual bool terminate_iteration(void);
   
   explicit sstatic(void);
   
   virtual ~sstatic(void);
};

}

#endif