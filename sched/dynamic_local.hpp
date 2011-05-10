
#ifndef SCHED_DYNAMIC_LOCAL_HPP
#define SCHED_DYNAMIC_LOCAL_HPP

#include <tr1/unordered_set>

#include "sched/static_local.hpp"
#include "sched/steal_set.hpp"

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
   
   void add_node(db::node *);
   void remove_node(db::node *);
   virtual void generate_aggs(void);
   virtual bool busy_wait(void);
   dynamic_local *select_steal_target(void) const;
   void handle_stealing(void);
   void change_node(thread_node *, dynamic_local *);
   
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