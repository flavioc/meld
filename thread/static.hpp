
#ifndef THREAD_STATIC_HPP
#define THREAD_STATIC_HPP

#include <boost/thread/barrier.hpp>
#include <vector>

#include "sched/base.hpp"
#include "sched/nodes/thread.hpp"
#include "queue/safe_linear_queue.hpp"
#include "sched/thread/threaded.hpp"

namespace sched
{

class static_local: public sched::base,
                    public sched::threaded
{
protected:
   
	queue::push_safe_linear_queue<thread_node*> queue_nodes;
   
   thread_node *current_node;
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   bool set_next_node(void);
   bool check_if_current_useless();
   void make_active(void);
   void make_inactive(void);
   virtual void generate_aggs(void);
   virtual bool busy_wait(void);
   
   inline void add_to_queue(thread_node *node)
   {
      queue_nodes.push(node);
   }
   
   inline bool has_work(void) const { return !queue_nodes.empty(); }
   
public:
   
   virtual void init(const size_t);
   
   virtual void new_agg(process::work&);
   virtual void new_work(const db::node *, process::work&);
   virtual void new_work_other(sched::base *, process::work&);
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
   
   virtual bool get_work(process::work&);
   virtual void finish_work(const process::work&);
   virtual void end(void);
   virtual bool terminate_iteration(void);
   
   static_local *find_scheduler(const db::node *);
   virtual db::simple_tuple_vector gather_active_tuples(db::node *, const vm::predicate_id);
	
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_node(id, trans);
   }
   
   static std::vector<sched::base*>& start(const size_t);
   
   virtual void write_slice(statistics::slice&) const;
   
   explicit static_local(const vm::process_id);
   
   virtual ~static_local(void);
};

}

#endif
