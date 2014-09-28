
#ifndef SCHED_BASE_HPP
#define SCHED_BASE_HPP

#include <iostream>
#include <list>
#include <stdexcept>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "conf.hpp"

#include "db/tuple.hpp"
#include "db/node.hpp"
#include "db/database.hpp"
#include "vm/state.hpp"
#include "utils/macros.hpp"
#include "stat/slice.hpp"
#include "process/work.hpp"
#include "stat/stat.hpp"
#include "vm/state.hpp"
#include "vm/temporary.hpp"
#include "mem/thread.hpp"

namespace process {
   class remote;
}

namespace sched
{

extern pthread_key_t sched_key;

class base
{
protected:
   
   const vm::process_id id;
   boost::thread *thread;
   
   size_t iteration;
   
#ifdef INSTRUMENTATION
   mutable statistics::sched_state ins_state;
   
#define ins_active ins_state = statistics::NOW_ACTIVE
#define ins_idle ins_state = statistics::NOW_IDLE
#define ins_sched ins_state = statistics::NOW_SCHED
#define ins_round ins_state = statistics::NOW_ROUND

#else

#define ins_active
#define ins_idle
#define ins_sched
#define ins_round

#endif

   void do_loop(void);
   void loop(void);

   void do_agg_tuple_add(db::node *, vm::tuple *, const vm::derivation_count);
   void do_tuple_add(db::node *, vm::tuple *, const vm::derivation_count);
   
   virtual void killed_while_active(void) { };
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

   inline void setup_node(db::node *node)
   {
      vm::predicate *init_pred(vm::theProgram->get_init_predicate());
      vm::tuple *init_tuple(vm::tuple::create(init_pred));
#ifdef FACT_STATISTICS
      state.facts_derived++;
#endif
      node->set_owner(this);
      node->add_linear_fact(init_tuple, init_pred);
      node->unprocessed_facts = true;
   }
   
public:
   
   inline bool leader_thread(void) const { return get_id() == 0; }

   db::node* init_node(db::database::map_nodes::iterator it)
   {
      db::node *node(vm::All->DATABASE->create_node_iterator(it));
#ifdef USE_REAL_NODES
      vm::theProgram->fix_node_address(node);
#endif
      setup_node(node);
      return node;
   }

   db::node* create_node(void)
   {
      db::node *node(vm::All->DATABASE->create_node());
      setup_node(node);
      return node;
   }
   
   // a new aggregate is to be inserted into the work queue
   inline void new_work_agg(db::node *node, db::simple_tuple *stpl)
   {
      process::work work(node, stpl, process::mods::LOCAL_TUPLE | process::mods::FORCE_AGGREGATE);
      new_agg(work);
   }
   
   // work to be sent to the same thread
   virtual void new_work(db::node *, db::node *, vm::tuple*, vm::predicate *, const vm::ref_count, const vm::depth_t) = 0;
   // delayed work to be sent to the target thread
   virtual void new_work_delay(db::node *, db::node *, vm::tuple*, vm::predicate *, const vm::ref_count, const vm::depth_t, const vm::uint_val)
   {
      assert(false);
   }

   // new aggregate
   virtual void new_agg(process::work&) = 0;
#ifdef COMPILE_MPI
   // work to be sent to a MPI process
   virtual void new_work_remote(process::remote *, const db::node::node_id, sched::message *) = 0;
#endif
   
   // ACTIONS
   virtual void set_node_priority(db::node *, const double) { }
	virtual void add_node_priority(db::node *, const double) { }
   virtual void schedule_next(db::node *) { }

	// GATHER QUEUE FACTS FROM NODE
   virtual void gather_next_tuples(db::node *, db::simple_tuple_list&) { }

   virtual void init(const size_t) = 0;
   virtual void end(void) = 0;
   
   virtual db::node *get_work(void) = 0;
   
   virtual void assert_end(void) const = 0;
   virtual void assert_end_iteration(void) const = 0;
   
   inline bool end_iteration(void)
   {
      ++iteration;
      return terminate_iteration();
   }
   
   inline vm::process_id get_id(void) const { return id; }
   
   inline size_t num_iterations(void) const { return iteration; }

	inline void join(void) { thread->join(); }
	
	void start(void);
   
   virtual void write_slice(statistics::slice& sl)
   {
#ifdef INSTRUMENTATION
      sl.state = ins_state;
      sl.consumed_facts = state.instr_facts_consumed;
      sl.derived_facts = state.instr_facts_derived;
      sl.rules_run = state.instr_rules_run;
      
      // reset stats
      state.instr_facts_consumed = 0;
      state.instr_facts_derived = 0;
      state.instr_rules_run = 0;
#else
      (void)sl;
#endif
   }

   static volatile bool stop_flag;

   static base* get_scheduler(void);
   
	explicit base(const vm::process_id);
   
	virtual ~base(void);
};

}

#endif
