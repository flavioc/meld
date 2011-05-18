
#ifndef SCHED_BASE_HPP
#define SCHED_BASE_HPP

#include "conf.hpp"

#include "db/tuple.hpp"
#include "db/node.hpp"
#include "vm/state.hpp"
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
   
   inline void node_iteration(db::node *node)
   {
      db::simple_tuple_list ls(node->end_iteration());

      for(db::simple_tuple_list::iterator it(ls.begin());
         it != ls.end();
         ++it)
      {
         new_work_agg(node, *it);
      }
   }
   
   inline void init_node(db::node *node)
   {
      db::simple_tuple *stpl(db::simple_tuple::create_new(new vm::tuple(vm::state::PROGRAM->get_init_predicate())));
      new_work_self(node, stpl, false);
   }
   
public:
   
   // a new work was created for the current executing node
   inline void new_work_self(db::node *node, const db::simple_tuple *tpl, const bool is_agg = false)
   {
      node->new_auto_tuple(tpl);
      new_work(node, node, tpl, is_agg);
   }
   
   // a new aggregate is to be inserted into the work queue
   inline void new_work_agg(db::node *node, const db::simple_tuple *tpl)
   {
      new_work_self(node, tpl, true);
   }
   
   // work to be sent to the same thread
   virtual void new_work(db::node *, db::node *, const db::simple_tuple *, const bool is_agg = false) = 0;
   // work to be sent to a different thread
   virtual void new_work_other(sched::base *, db::node *, const db::simple_tuple *) = 0;
   // work to be sent to a MPI process
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
