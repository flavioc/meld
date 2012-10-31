
#ifndef THREAD_DIRECT_HPP
#define THREAD_DIRECT_HPP

#include <tr1/unordered_set>

#include "conf.hpp"
#include "queue/safe_linear_queue.hpp"
#include "utils/spinlock.hpp"
#include "sched/nodes/thread.hpp"
#include "sched/thread/threaded.hpp"
#include "utils/random.hpp"

namespace sched
{
   
class direct_local: public sched::base,
                    public sched::threaded
{
private:
	queue::safe_linear_queue<thread_node*> queue_nodes;
   
   thread_node *current_node;
   mutable utils::randgen random;
   bool added_any;
   
#ifdef MARK_OWNED_NODES
   typedef std::tr1::unordered_set<db::node*, std::tr1::hash<db::node*>,
                     std::equal_to<db::node*>, mem::allocator<db::node*> > node_set;
   
   node_set *nodes;

   utils::spinlock *nodes_mutex;
#endif

#ifdef INSTRUMENTATION
   mutable utils::atomic<size_t> stealed_nodes;
#endif
   
   virtual bool busy_wait(void);
   direct_local *select_steal_target(void) const;
   
   inline bool has_work(void) const { return !queue_nodes.empty(); }
   inline void add_to_queue(thread_node *node)
   {
      queue_nodes.push(node);
   }
   
   bool check_if_current_useless();
   bool set_next_node(void);
   void try_to_steal(void);
   
protected:
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   virtual void generate_aggs(void);
#ifdef MARK_OWNED_NODES
   void add_node(db::node *);
   void remove_node(db::node *);
#endif
   virtual void new_agg(process::work&);
   virtual void new_work(const db::node *, process::work&);
   virtual void new_work_other(sched::base *, process::work&);
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *)
   {
      assert(false);
   }
   virtual void change_node(thread_node *, direct_local *);
   
public:
   
   virtual void init(const size_t);
   virtual void end(void);
   virtual bool terminate_iteration(void);
   
   virtual bool get_work(process::work&);
   
   virtual void write_slice(statistics::slice& sl) const;
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_node(id, trans);
   }
   
   direct_local *find_scheduler(const db::node *);
   
   static std::vector<sched::base*>& start(const size_t);
   
   explicit direct_local(const vm::process_id);
   
   virtual ~direct_local(void);
};

}
#endif
