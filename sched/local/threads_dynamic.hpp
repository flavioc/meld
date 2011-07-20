
#ifndef SCHED_LOCAL_THREADS_DYNAMIC_HPP
#define SCHED_LOCAL_THREADS_DYNAMIC_HPP

#include <tr1/unordered_set>

#include "conf.hpp"
#include "sched/local/threads_static.hpp"
#include "sched/thread/steal_set.hpp"
#include "utils/spinlock.hpp"

namespace sched
{
   
class dynamic_local: public static_local
{
private:
   DEFINE_PADDING;
   
   typedef std::tr1::unordered_set<db::node*, std::tr1::hash<db::node*>,
                     std::equal_to<db::node*>, mem::allocator<db::node*> > node_set;
   
   node_set *nodes;

   utils::spinlock *nodes_mutex;
   
   DEFINE_PADDING;
   
   steal_set steal;
	
   DEFINE_PADDING;
   
	size_t asked_many;
	
#ifdef INSTRUMENTATION
   mutable utils::atomic<size_t> stealed_nodes;
   mutable utils::atomic<size_t> steal_requests;
#endif
   
   virtual bool busy_wait(void);
   void handle_stealing(void);
   
protected:
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   virtual void generate_aggs(void);
   dynamic_local *select_steal_target(void) const;
   void add_node(db::node *);
   void remove_node(db::node *);
   void request_work_to(dynamic_local *);
   virtual void change_node(thread_node *, dynamic_local *);
   
public:
   
   virtual void init(const size_t);
   virtual void end(void);
   
   virtual bool get_work(process::work&);
   
   virtual void write_slice(stat::slice& sl) const;
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_node(id, trans);
   }
   
   dynamic_local *find_scheduler(const db::node *);
   
   static std::vector<sched::base*>& start(const size_t);
   
   explicit dynamic_local(const vm::process_id);
   
   virtual ~dynamic_local(void);
};

}
#endif
