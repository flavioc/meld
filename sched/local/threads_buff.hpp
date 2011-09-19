
#ifndef SCHED_LOCAL_THREADS_BUFF_HPP
#define SCHED_LOCAL_THREADS_BUFF_HPP

#include <map>
#include <tr1/unordered_map>

#include "sched/local/threads_static.hpp"
#include "sched/queue/unsafe_queue.hpp"
#include "sched/queue/unsafe_queue_count.hpp"

namespace sched
{

class static_buff: public sched::static_local
{
protected:
   
   DEFINE_PADDING;
   
   size_t step_receive;
   size_t round_trip_receive;
   size_t step_send;
   size_t round_trip_send;
   
   DEFINE_PADDING;
   
   typedef unsafe_queue_count<process::work> queue_buffer;
   typedef std::pair<const sched::base*, queue_buffer> map_pair;
   typedef std::tr1::unordered_map<sched::base*, queue_buffer,
      std::tr1::hash<sched::base*>, std::equal_to<sched::base*>, mem::allocator< map_pair > > map_buffer;
      
   map_buffer tuples_buf;
   
   DEFINE_PADDING;
   
   safe_queue<process::work> incoming;
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   
   virtual bool busy_wait(void);
   
   void flush_buffer(void);
   
   inline bool has_incoming_work(void) const { return !incoming.empty(); }
   inline bool has_any_work(void) const { return has_work() || has_incoming_work(); }
   
   void process_incoming(void);
   
public:
   
   virtual void new_agg(process::work&);
   virtual void new_work(const db::node *, process::work&);
   virtual void new_work_other(sched::base *, process::work&);
   
   virtual bool get_work(process::work&);
   
   static db::node *create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return new db::node(id, trans);
   }
   
   static std::vector<sched::base*>& start(const size_t);
   
   virtual void write_slice(stat::slice&) const;
   
   explicit static_buff(const vm::process_id);
   
   virtual ~static_buff(void);
};

}

#endif
