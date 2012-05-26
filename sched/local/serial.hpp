
#ifndef SCHED_LOCAL_SERIAL_HPP
#define SCHED_LOCAL_SERIAL_HPP

#include "sched/base.hpp"
#include "sched/nodes/serial.hpp"
#include "sched/queue/unsafe_queue.hpp"

namespace sched
{
   
class serial_local: public sched::base
{
protected:
	
	serial_node *current_node;
	unsafe_queue<serial_node*> queue_nodes;
	
private:
   
   inline bool has_work(void) const { return !queue_nodes.empty(); }
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   virtual void generate_aggs(void);
   
public:
   
   virtual void new_agg(process::work&);
   virtual void new_work(const db::node *, process::work&);
   
   virtual void new_work_other(sched::base *, process::work&)
   {
      assert(false);
   }
   
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *)
   {
      assert(false);
   }
   
   virtual bool get_work(process::work&);
   
   virtual void init(const size_t);
   virtual void end(void) {}
   virtual bool terminate_iteration(void);
   
   static db::node* create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return dynamic_cast<db::node*>(new sched::serial_node(id, trans));
   }
   
   serial_local *find_scheduler(const db::node *) { return this; }
   
   explicit serial_local(void):
      sched::base(0),
      current_node(NULL)
   {
   }
   
   virtual ~serial_local(void) {}
};

}

#endif