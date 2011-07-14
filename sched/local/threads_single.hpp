
#ifndef SCHED_LOCAL_THREADS_SINGLE_HPP
#define SCHED_LOCAL_THREADS_SINGLE_HPP

#include <boost/thread/barrier.hpp>
#include <vector>

#include "sched/base.hpp"
#include "sched/thread/node.hpp"
#include "sched/queue/safe_queue_multi.hpp"
#include "sched/thread/threaded.hpp"
#include "sched/thread/queue_buffer.hpp"

namespace sched
{

class threads_single: public sched::base,
                      public sched::threaded
{
protected:
   
   static safe_queue_multi<thread_node*> queue_nodes;
   
   DEFINE_PADDING;
   
   thread_node *current_node;
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   bool set_next_node(void);
   bool check_if_current_useless();
   void make_active(void);
   void make_inactive(void);
   virtual void generate_aggs(void);
   virtual bool busy_wait(void);
   
   static inline bool has_work(void) { return !queue_nodes.empty(); }
   
   static inline void add_to_queue(thread_node *node)
   {
      queue_nodes.push(node);
   }
   
public:
   
   virtual void init(const size_t);
   
   virtual void new_work(db::node *, db::node *, const db::simple_tuple*, const bool is_agg = false);
   virtual void new_work_other(sched::base *, db::node *, const db::simple_tuple *);
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *);
   
   virtual bool get_work(work_unit&);
   virtual void finish_work(const work_unit&);
   virtual void end(void) {}
   virtual bool terminate_iteration(void);
   
   threads_single *find_scheduler(const db::node::node_id);
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_node(id, trans);
   }
   
   static std::vector<sched::base*>& start(const size_t);
   
   virtual void write_slice(stat::slice&) const;
   
   explicit threads_single(const vm::process_id);
   
   virtual ~threads_single(void);
};

}

#endif
