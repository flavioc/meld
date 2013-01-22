
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
   virtual void new_work(const db::node *, process::work&);
   
   virtual void new_work_other(sched::base *, process::work&)
   {
      assert(false);
   }
   
#ifdef COMPILE_MPI
   virtual void new_work_remote(process::remote *, const db::node::node_id, message *)
   {
      assert(false);
   }
#endif
   
	virtual db::simple_tuple_vector gather_active_tuples(db::node *, const vm::predicate_id);
   virtual void gather_next_tuples(db::node *, db::simple_tuple_list&);
   
   virtual db::node* get_work(void);
   
   virtual void init(const size_t);
   virtual void end(void) {}
   virtual bool terminate_iteration(void);
   
   static db::node* create_node(const db::node::node_id id, const db::node::node_id trans, vm::all *all)
   {
      return dynamic_cast<db::node*>(new sched::serial_node(id, trans, all));
   }
   
   serial_local *find_scheduler(const db::node *) { return this; }
   
   explicit serial_local(vm::all *all):
      sched::base(0, all),
      current_node(NULL)
   {
   }
   
   virtual ~serial_local(void) {}
};

}

#endif
