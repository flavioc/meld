
#ifndef THREAD_THREADS_HPP
#define THREAD_THREADS_HPP

#include <boost/thread/barrier.hpp>
#include <vector>

#include "sched/base.hpp"
#include "sched/nodes/thread.hpp"
#include "queue/safe_linear_queue.hpp"
#include "sched/thread/threaded.hpp"
#include "sched/nodes/thread_intrusive.hpp"
#include "queue/safe_complex_pqueue.hpp"
#include "queue/safe_double_queue.hpp"
#include "utils/random.hpp"

namespace sched
{

class threads_sched: public sched::base,
                    public sched::threaded
{
protected:
   
   typedef queue::intrusive_safe_double_queue<thread_intrusive_node> node_queue;
   node_queue queue_nodes;
   
   thread_intrusive_node *current_node;
#ifdef TASK_STEALING
   mutable utils::randgen rand;
   size_t next_thread;
   size_t backoff;
#endif

#ifdef INSTRUMENTATION
   size_t sent_facts_same_thread;
   size_t sent_facts_other_thread;
   size_t sent_facts_other_thread_now;
#endif

#ifdef TASK_STEALING
#ifdef INSTRUMENTATION
   size_t stolen_total;
#endif

   void clear_steal_requests(void);
   bool go_steal_nodes(void);
   virtual thread_intrusive_node* steal_node(void);
   virtual size_t number_of_nodes(void) const;
   virtual void check_stolen_node(thread_intrusive_node *) {};
#endif
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   bool set_next_node(void);
   virtual bool check_if_current_useless();
   void make_active(void);
   void make_inactive(void);
   virtual void generate_aggs(void);
   virtual bool busy_wait(void);
   
   virtual void add_to_queue(thread_intrusive_node *node)
   {
      queue_nodes.push_tail(node);
   }
   
   virtual bool has_work(void) const { return !queue_nodes.empty(); }

   virtual void killed_while_active(void);
   
public:
   
   virtual void init(const size_t);
   
   virtual void new_agg(process::work&);
   virtual void new_work(db::node *, db::node *, vm::tuple *, vm::predicate *, const vm::ref_count, const vm::depth_t);
#ifdef COMPILE_MPI
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
#endif
   
   virtual db::node* get_work(void);
   virtual void end(void);
   virtual bool terminate_iteration(void);
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_intrusive_node(id, trans);
   }
   
   DEFINE_START_FUNCTION(threads_sched);
   
   virtual void write_slice(statistics::slice&);
   
   explicit threads_sched(const vm::process_id);
   
   virtual ~threads_sched(void);
};

}

#endif
