
#ifndef SCHED_LOCAL_THREADS_BUFF_HPP
#define SCHED_LOCAL_THREADS_BUFF_HPP

#include <map>
#include <tr1/unordered_map>

#include "sched/local/threads_static.hpp"
#include "queue/unsafe_queue.hpp"
#include "queue/unsafe_queue_count.hpp"
#include "sched/nodes/unsafe_static.hpp"

namespace sched
{

class static_buff: public sched::base,
                  public sched::threaded
{
protected:
   
   DEFINE_PADDING;
   
   unsafe_queue<unsafe_static_node*> queue_nodes;
   
   DEFINE_PADDING;
   
   unsafe_static_node *current_node;
   
   DEFINE_PADDING;
   
   size_t step_fetch;
   size_t round_trip_receive;
   size_t step_send;
   size_t round_trip_send;

   DEFINE_PADDING;
   
   typedef unsafe_queue<process::work> queue_buffer;
   typedef std::pair<const sched::base*, queue_buffer> map_pair;
   typedef std::tr1::unordered_map<sched::base*, queue_buffer,
      std::tr1::hash<sched::base*>, std::equal_to<sched::base*>, mem::allocator< map_pair > > map_buffer;
      
   map_buffer tuples_buf;
   
   DEFINE_PADDING;
   
   safe_queue<process::work> incoming;
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   
   bool set_next_node(void);
   void make_active(void);
   void make_inactive(void);
   virtual void generate_aggs(void);
   
   virtual bool busy_wait(void);
   
   void flush_buffer(void);
   
   inline void add_to_queue(unsafe_static_node *node)
   {
      queue_nodes.push(node);
   }
   
   inline bool has_work(void) const { return !queue_nodes.empty(); }
   inline bool has_incoming_work(void) const { return !incoming.empty(); }
   inline bool has_any_work(void) const { return has_work() || has_incoming_work(); }
   
   void process_incoming(void);
   
public:
   
   virtual void new_agg(process::work&);
   virtual void new_work(const db::node *, process::work&);
   virtual void new_work_other(sched::base *, process::work&);
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *) { assert(false); }
   
   virtual bool get_work(process::work&);
   
   static_buff *find_scheduler(const db::node *);
   
   virtual void init(const size_t);
   virtual void end(void) {}
   virtual bool terminate_iteration(void);
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new unsafe_static_node(id, trans);
   }
   
   static std::vector<sched::base*>& start(const size_t);
   
   virtual void write_slice(statistics::slice&) const;
   
   explicit static_buff(const vm::process_id);
   
   virtual ~static_buff(void);
};

}

#endif
