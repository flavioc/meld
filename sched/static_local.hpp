
#ifndef SCHED_STATIC_LOCAL_HPP
#define SCHED_STATIC_LOCAL_HPP

#include <vector>
#include <tr1/unordered_set>

#include "sched/base.hpp"
#include "sched/node.hpp"
#include "sched/queue.hpp"

namespace sched
{

class static_local: public sched::base
{
private:
   
   utils::byte _pad_threads1[128];
   
   enum {
      PROCESS_ACTIVE,
      PROCESS_INACTIVE
   } process_state;
   boost::mutex mutex;
   
   utils::byte _pad_threads2[128];
   
   safe_queue<thread_node*> queue_nodes;
   thread_node *current_node;
   
   utils::byte _pad_threads3[128];
   
   typedef std::tr1::unordered_set<db::node*, std::tr1::hash<db::node*>,
                     std::equal_to<db::node*>, mem::allocator<db::node*> > node_set;
   
   node_set *nodes;
   boost::mutex *nodes_mutex;
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   bool set_next_node(void);
   bool check_if_current_useless();
   void make_active(void);
   void make_inactive(void);
   void generate_aggs(void);
   bool busy_wait(void);
   void add_node(db::node *);
   void remove_node(db::node *);
   static_local *select_steal_target(void) const;
   inline void add_to_queue(thread_node *node) {
      node->set_in_queue(true);
      queue_nodes.push(node);
   }
   
public:
   
   virtual void init(const size_t);
   
   virtual void new_work(db::node *, db::node *, const db::simple_tuple*, const bool is_agg = false);
   virtual void new_work_other(sched::base *, db::node *, const db::simple_tuple *);
   virtual void new_work_remote(process::remote *, const vm::process_id, process::message *);
   
   virtual bool get_work(work_unit&);
   virtual void finish_work(const work_unit&);
   virtual void end(void);
   virtual bool terminate_iteration(void);
   
   static_local *find_scheduler(const db::node::node_id);
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new thread_node(id, trans);
   }
   
   static std::vector<static_local*>& start(const size_t);
   
   explicit static_local(const vm::process_id);
   
   virtual ~static_local(void);
};

}

#endif
