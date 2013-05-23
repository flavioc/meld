
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
#include "vm/state.hpp"
#include "sched/mpi/message.hpp"
#include "utils/macros.hpp"
#include "stat/slice.hpp"
#include "process/work.hpp"
#include "stat/stat.hpp"
#include "runtime/string.hpp"
#include "vm/state.hpp"

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
   vm::state state;
   
   size_t iteration;
   
#ifdef INSTRUMENTATION
   mutable utils::atomic<size_t> processed_facts;
   mutable utils::atomic<size_t> sent_facts;
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
   void do_work(db::node *);
   void do_agg_tuple_add(db::node *, vm::tuple *, const vm::ref_count);
   void do_tuple_add(db::node *, vm::tuple *, const vm::ref_count);
   
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
   
public:
   
   inline bool leader_thread(void) const { return get_id() == 0; }

   virtual void init_node(db::node *node)
   {
      db::simple_tuple *stpl(db::simple_tuple::create_new(new vm::tuple(state.all->PROGRAM->get_init_predicate())));
      new_work_self(node, stpl);
      node->init();
      node->set_owner(this);
   }
   
   // a new work was created for the current executing node
   inline void new_work_self(db::node *node, db::simple_tuple *stpl, const process::work_modifier mod = process::mods::NOTHING)
   {
      process::work work(node, stpl, process::mods::LOCAL_TUPLE | mod);
      new_work(node, work);
   }
   
   // a new aggregate is to be inserted into the work queue
   inline void new_work_agg(db::node *node, db::simple_tuple *stpl)
   {
      process::work work(node, stpl, process::mods::LOCAL_TUPLE | process::mods::FORCE_AGGREGATE);
      new_agg(work);
   }
   
   // work to be sent to the same thread
   virtual void new_work(const db::node *from, process::work&) = 0;
   // delayed work to be sent to the target thread
   virtual void new_work_delay(sched::base *, const db::node *, process::work&, const vm::uint_val)
   {
      assert(false);
   }

   // new aggregate
   virtual void new_agg(process::work&) = 0;
   // work to be sent to a different thread
   virtual void new_work_other(sched::base *, process::work&) = 0;
#ifdef COMPILE_MPI
   // work to be sent to a MPI process
   virtual void new_work_remote(process::remote *, const db::node::node_id, sched::message *) = 0;
#endif
   
   // ACTIONS
   virtual void set_node_priority(db::node *, const double) { }
	virtual void add_node_priority(db::node *, const double) { }
   virtual void add_node_priority_other(db::node *, const double) { }
   virtual void set_node_priority_other(db::node *, const double) { }
   virtual void schedule_next(db::node *) { }

	// GATHER QUEUE FACTS FROM NODE
	virtual db::simple_tuple_vector gather_active_tuples(db::node *, const vm::predicate_id) { return db::simple_tuple_vector(); }
   virtual void gather_next_tuples(db::node *, db::simple_tuple_list&) { }

   virtual void init(const size_t) = 0;
   virtual void end(void) = 0;
   
   virtual db::node *get_work(void) = 0;
   
   virtual void finish_work(db::node *)
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
   
   virtual base* find_scheduler(const db::node*) = 0;
   
   inline vm::process_id get_id(void) const { return id; }
   
   inline size_t num_iterations(void) const { return iteration; }

	inline void join(void) { thread->join(); }
	
	void start(void);
   
   virtual void write_slice(statistics::slice& sl) const
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

   static base* get_scheduler(void);
   
	explicit base(const vm::process_id, vm::all *);
   
	virtual ~base(void);
};

}

#endif
