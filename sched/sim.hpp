
#ifndef SCHED_SIM_HPP
#define SCHED_SIM_HPP

#include <boost/asio.hpp>
#include <list>

#include "sched/base.hpp"
#include "sched/nodes/sim.hpp"
//#include "queue/unsafe_double_queue.hpp"
//#include "queue/intrusive.hpp"

namespace sched
{
   
class sim_sched: public sched::base
{
protected:
	
	sim_node *current_node;
	boost::asio::ip::tcp::socket *socket;
	std::list<process::work> tmp_work;
	vm::predicate *neighbor_pred;
	vm::predicate *tap_pred;
	
private:
   
   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   virtual void generate_aggs(void);
   
public:
	
	static int PORT;
   
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
      return dynamic_cast<db::node*>(new sched::sim_node(id, trans, all));
   }

	void set_color(db::node *, const int, const int, const int);
   
   sim_sched *find_scheduler(const db::node *) { return this; }
   
   explicit sim_sched(vm::all *all):
      sched::base(0, all),
      current_node(NULL),
		socket(NULL)
   {
   }
   
	virtual ~sim_sched(void);
};

}

#endif
