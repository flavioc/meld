
#ifndef SCHED_LOCAL_THREADS_DYNAMIC_HPP
#define SCHED_LOCAL_THREADS_DYNAMIC_HPP

#include <tr1/unordered_set>

#include "sched/local/threads_static.hpp"
#include "sched/thread/steal_set.hpp"

namespace sched
{
   
class dynamic_local: public static_local
{
private:
   utils::byte _paddl1[128];
   
   typedef std::tr1::unordered_set<db::node*, std::tr1::hash<db::node*>,
                     std::equal_to<db::node*>, mem::allocator<db::node*> > node_set;
   
   node_set *nodes;
   boost::mutex *nodes_mutex;
   
   utils::byte _paddl2[128];
   steal_set steal;
   
   virtual bool busy_wait(void);
   void handle_stealing(void);
   
protected:
   
   virtual void generate_aggs(void);
   dynamic_local *select_steal_target(void) const;
   void add_node(db::node *);
   void remove_node(db::node *);
   void request_work_to(dynamic_local *);
   virtual void change_node(thread_node *, dynamic_local *);
   
public:
   
   virtual void init(const size_t);
   virtual void end(void);
   
   virtual bool get_work(work_unit&);
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_node(id, trans);
   }
   
   static std::vector<sched::base*>& start(const size_t);
   
   explicit dynamic_local(const vm::process_id);
   
   virtual ~dynamic_local(void);
};

}
#endif
