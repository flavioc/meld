
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
#include "process/work.hpp"
#include "stat/stat.hpp"

namespace process {
   class remote;
}

namespace sched
{

class base
{
protected:
   
   const vm::process_id id;
   
   DEFINE_PADDING;
   
   size_t iteration;
   
#ifdef INSTRUMENTATION
   mutable utils::atomic<size_t> processed_facts;
   mutable utils::atomic<size_t> sent_facts;
   mutable stat::sched_state ins_state;
   
#define ins_active ins_state = stat::NOW_ACTIVE
#define ins_idle ins_state = stat::NOW_IDLE
#define ins_sched ins_state = stat::NOW_SCHED
#define ins_round ins_state = stat::NOW_ROUND
#define ins_comm ins_state = stat::NOW_COMM

#else

#define ins_active
#define ins_idle
#define ins_sched
#define ins_round
#define ins_comm

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
      new_work_self(node, stpl);
      node->init();
   }
   
public:
   
   inline bool leader_thread(void) const { return get_id() == 0; }
   
   // a new work was created for the current executing node
   inline void new_work_self(db::node *node, db::simple_tuple *stpl, const process::work_modifier mod = process::mods::NOTHING)
   {
      process::work work(node, stpl, process::mods::LOCAL_TUPLE | mod);
      node->push_auto(stpl);
      new_work(node, work);
   }
   
   // a new aggregate is to be inserted into the work queue
   inline void new_work_agg(db::node *node, db::simple_tuple *stpl)
   {
      process::work work(node, stpl, process::mods::LOCAL_TUPLE | process::mods::FORCE_AGGREGATE);
      node->push_auto(stpl);
      new_agg(work);
   }
   
   // work to be sent to the same thread
   virtual void new_work(const db::node *, process::work&) = 0;
   // new aggregate
   virtual void new_agg(process::work&) = 0;
   // work to be sent to a different thread
   virtual void new_work_other(sched::base *, process::work&) = 0;
   // work to be sent to a MPI process
   virtual void new_work_remote(process::remote *, const db::node::node_id, sched::message *) = 0;
   
   // ACTIONS
   virtual void set_node_priority(db::node *, const int) { printf("set prio\n"); }
   
   virtual void init(const size_t) = 0;
   virtual void end(void) = 0;
   
   virtual bool get_work(process::work&) = 0;
   
   virtual void finish_work(const process::work&)
   {
#ifdef INSTRUMENTATION
      processed_facts++;
#endif
   }
   
#ifdef INSTRUMENTATION
   inline void instr_set_active(void) { ins_active; }
   inline void instr_set_comm(void) { ins_comm; }
#define sched_active(SCHED) (SCHED)->instr_set_active()
#define sched_comm(SCHED)   (SCHED)->instr_set_comm()
#else
#define sched_active(SCHED)
#define sched_comm(SCHED)
#endif
   
   virtual void assert_end(void) const = 0;
   virtual void assert_end_iteration(void) const = 0;
   
   inline bool end_iteration(void)
   {
      ++iteration;
      return terminate_iteration();
   }
   
   virtual base* find_scheduler(const db::node*) = 0;
   
   inline vm::process_id get_id(void) const { return id; }
   
   inline size_t num_iterations(void) const { return iteration; }
   
   virtual void write_slice(stat::slice& sl) const
   {
#ifdef INSTRUMENTATION
      sl.state = ins_state;
      sl.processed_facts = processed_facts;
      sl.sent_facts = sent_facts;
      
      // reset stats
      processed_facts = 0;
      sent_facts = 0;
#else
      (void)sl;
#endif
   }
   
   explicit base(const vm::process_id _id):
      id(_id), iteration(0)
#ifdef INSTRUMENTATION
      , processed_facts(0), sent_facts(0), ins_state(stat::NOW_ACTIVE)
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
