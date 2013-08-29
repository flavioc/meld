
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

#define TASK_STEALING 1
#define STEALING_ROUND_MAX 1000000

namespace sched
{

class threads_sched: public sched::base,
                    public sched::threaded
{
protected:
   
   typedef queue::intrusive_safe_double_queue<thread_intrusive_node> node_queue;
   node_queue queue_nodes;
   queue::push_safe_linear_queue<process::work> buffer;
   
   thread_intrusive_node *current_node;
   mutable utils::randgen rand;

   queue::push_safe_linear_queue<thread_intrusive_node*> stolen_nodes_buffer;

#ifdef TASK_STEALING
#ifdef INSTRUMENTATION
   mutable size_t stolen_total;
   mutable size_t steal_requests;
#endif

   void clear_steal_requests(void);
   void go_steal_nodes(void);
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
   
   virtual bool has_work(void) const { return !queue_nodes.empty() || !buffer.empty() || !stolen_nodes_buffer.empty(); }
   
public:
   
   virtual void init(const size_t);
   
   virtual void new_agg(process::work&);
   virtual void new_work(const db::node *, process::work&);
   virtual void new_work_other(sched::base *, process::work&);
#ifdef COMPILE_MPI
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
#endif
   
   virtual db::node* get_work(void);
   virtual void finish_work(db::node *);
   virtual void end(void);
   virtual bool terminate_iteration(void);
   
   threads_sched *find_scheduler(const db::node *);
   virtual void gather_next_tuples(db::node *, db::simple_tuple_list&);
	
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans, vm::all *all)
   {
      return new thread_intrusive_node(id, trans, all);
   }
   
   DEFINE_START_FUNCTION(threads_sched);
   
   virtual void write_slice(statistics::slice&) const;
   
   explicit threads_sched(const vm::process_id, vm::all *);
   
   virtual ~threads_sched(void);
};

}

#endif
