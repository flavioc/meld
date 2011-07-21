
#ifndef SCHED_GLOBAL_SERIAL_HPP
#define SCHED_GLOBAL_SERIAL_HPP

#include <queue>

#include "sched/base.hpp"
#include "mem/allocator.hpp"
#include "sched/queue/bounded_pqueue.hpp"
#include "vm/predicate.hpp"

namespace sched
{
   
class serial_global: public sched::base
{
private:
   
   unsafe_bounded_pqueue<process::work>::type queue_work;
   
   void generate_aggs(void);
   
   inline bool has_work(void) const { return !queue_work.empty(); }
   
   virtual void assert_end_iteration(void) const;
   virtual void assert_end(void) const;
   
public:
   
   virtual void new_agg(process::work&);
   virtual void new_work(const db::node *, process::work&);
   
   virtual void new_work_other(sched::base *, process::work&)
   {
      assert(false);
   }
   
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *)
   {
      assert(false);
   }
   
   virtual bool terminate_iteration(void);
   
   virtual void init(const size_t);
   virtual void end(void);
   virtual bool get_work(process::work&);
   
   serial_global *find_scheduler(const db::node*) { return this; }
   
   static db::node* create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new db::node(id, trans);
   }
   
   explicit serial_global(void):
      sched::base(0),
      queue_work(vm::predicate::MAX_STRAT_LEVEL)
   {
   }
   
   virtual ~serial_global(void) {}
};

}
#endif