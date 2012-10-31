
#ifndef THREAD_DYNAMIC_HPP
#define THREAD_DYNAMIC_HPP

#include <tr1/unordered_set>

#include "conf.hpp"
#include "thread/static.hpp"
#include "sched/thread/steal_set.hpp"
#include "utils/spinlock.hpp"
#include "utils/random.hpp"

namespace sched
{
   
class dynamic_local: public static_local
{
private:
#ifdef MARK_OWNED_NODES
   typedef std::tr1::unordered_set<db::node*, std::tr1::hash<db::node*>,
                     std::equal_to<db::node*>, mem::allocator<db::node*> > node_set;
   
   node_set *nodes;

   utils::spinlock *nodes_mutex;
#endif

   steal_set steal;
	
   size_t next_steal_cycle;
   size_t num_nodes_to_send;
   mutable utils::randgen random;
   bool added_any;
	
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
#ifdef MARK_OWNED_NODES
   void add_node(db::node *);
   void remove_node(db::node *);
#else
   virtual void new_agg(process::work&);
#endif
   void request_work_to(dynamic_local *);
   virtual void change_node(thread_node *, dynamic_local *);
   void steal_nodes(size_t&);
   
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
   
   dynamic_local *find_scheduler(const db::node *);
   
   static std::vector<sched::base*>& start(const size_t);
   
   explicit dynamic_local(const vm::process_id);
   
   virtual ~dynamic_local(void);
};

}
#endif
