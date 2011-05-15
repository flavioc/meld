
#ifndef SCHED_BASE_HPP
#define SCHED_BASE_HPP

#include "conf.hpp"

#include "db/tuple.hpp"
#include "db/node.hpp"
#include "sched/mpi/message.hpp"

namespace process {
   class remote;
}

namespace sched
{

struct work_unit {
   db::node *work_node;
   const db::simple_tuple *work_tpl;
   bool agg;
};
   
class base
{
protected:
   
   const vm::process_id id;
   
   utils::byte _pad_base[128];
   
   size_t iteration;
   
   virtual bool terminate_iteration(void) = 0;
   
public:
   
   virtual void new_work(db::node *, db::node *, const db::simple_tuple *, const bool is_agg = false) = 0;
   virtual void new_work_other(sched::base *, db::node *, const db::simple_tuple *) = 0;
   virtual void new_work_remote(process::remote *, const db::node::node_id, sched::message *) = 0;
   
   virtual void init(const size_t) = 0;
   virtual void end(void) = 0;
   
   virtual bool get_work(work_unit&) = 0;
   virtual void finish_work(const work_unit&) = 0;
   
   virtual void assert_end(void) const = 0;
   virtual void assert_end_iteration(void) const = 0;
   
   inline bool end_iteration(void)
   {
      ++iteration;
      return terminate_iteration();
   }
   
   virtual base* find_scheduler(const db::node::node_id) = 0;
   
   inline const vm::process_id get_id(void) const { return id; }
   
   explicit base(const vm::process_id _id): id(_id), iteration(0) {}
   
   virtual ~base(void) {}
};

}

#endif
