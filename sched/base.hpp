
#ifndef SCHED_BASE_HPP
#define SCHED_BASE_HPP

#include <iostream>

#include "conf.hpp"

#include "db/tuple.hpp"
#include "db/node.hpp"
#include "vm/state.hpp"
#include "sched/mpi/message.hpp"
#include "utils/macros.hpp"
#include "stat/slice.hpp"

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

struct node_work_unit {
   const db::simple_tuple *work_tpl;
   bool agg;
};

class base
{
protected:
   
   const vm::process_id id;
   
   DEFINE_PADDING;
   
   size_t iteration;
   
#ifdef INSTRUMENTATION
   mutable utils::atomic<size_t> processed_facts;
   mutable utils::atomic<size_t> sent_facts;
#endif
   
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
      node->init();
   }
   
public:
   
   inline const bool leader_thread(void) const { return get_id() == 0; }
   
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
   
   virtual void finish_work(const work_unit&)
   {
#ifdef INSTRUMENTATION
      processed_facts++;
#endif
   }
   
   virtual void assert_end(void) const = 0;
   virtual void assert_end_iteration(void) const = 0;
   
   inline bool end_iteration(void)
   {
      ++iteration;
      return terminate_iteration();
   }
   
   virtual base* find_scheduler(const db::node::node_id) = 0;
   
   inline const vm::process_id get_id(void) const { return id; }
   
   virtual void write_slice(stat::slice& sl) const
   {
#ifdef INSTRUMENTATION
      sl.processed_facts = processed_facts;
      sl.sent_facts = sent_facts;
      
      // reset stats
      processed_facts = 0;
      sent_facts = 0;
#endif
   }
   
   explicit base(const vm::process_id _id):
      id(_id), iteration(0)
#ifdef INSTRUMENTATION
      , processed_facts(0), sent_facts(0)
#endif
   {
   }
   
   virtual ~base(void)
   {
      //std::cout << iteration << std::endl;
   }
};

}

#endif
