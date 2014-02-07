
#ifndef SCHED_SERIAL_HPP
#define SCHED_SERIAL_HPP

#include "sched/base.hpp"
#include "sched/nodes/serial.hpp"
#include "queue/unsafe_double_queue.hpp"
#include "queue/intrusive.hpp"

namespace sched
{
   
class serial_local: public sched::base
{
protected:
	
	serial_node *current_node;
	queue::intrusive_unsafe_double_queue<serial_node> queue_nodes;
	
private:
   
   inline bool has_work(void) const { return !queue_nodes.empty(); }
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   virtual void generate_aggs(void);
   
public:
   
   virtual void new_agg(process::work&);
   virtual void new_work(db::node *, db::node *, vm::tuple*, vm::predicate *, const vm::ref_count, const vm::depth_t);
   
#ifdef COMPILE_MPI
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *)
   {
      assert(false);
   }
#endif
   
   virtual void gather_next_tuples(db::node *, db::simple_tuple_list&);
   
   virtual db::node* get_work(void);
   
   virtual void init(const size_t);
   virtual void end(void) {}
   virtual bool terminate_iteration(void);
   
   static db::node* create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return dynamic_cast<db::node*>(new sched::serial_node(id, trans));
   }
   
   explicit serial_local(void):
      sched::base(0),
      current_node(NULL)
   {
   }
   
   virtual ~serial_local(void) {}
};

}

#endif
