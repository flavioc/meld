
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
	
	typedef uint64_t message_type;
	static const size_t MAXLENGTH = 512 / sizeof(sim_sched::message_type);

   typedef struct {
      process::work work;
      size_t timestamp;
      sim_node *src;
   } work_info;
	
	sim_node *current_node;
	boost::asio::ip::tcp::socket *socket;
	std::list<work_info> tmp_work;
	static vm::predicate *neighbor_pred;
	static vm::predicate *tap_pred;
	static vm::predicate *neighbor_count_pred;
	static vm::predicate *accel_pred;
	static vm::predicate *shake_pred;
	static vm::predicate *vacant_pred;
	static bool thread_mode;
	static bool stop_all;
	static queue::push_safe_linear_queue<message_type*> socket_messages;
	bool slave;
	
private:

   virtual void assert_end(void) const;
   virtual void assert_end_iteration(void) const;
   virtual void generate_aggs(void);
   void send_pending_messages(void);
   void schedule_new_message(message_type *);
   void add_received_tuple(sim_node *, size_t, db::simple_tuple*);
   void add_vacant(const size_t, sim_node *, const int, const int);
   void add_neighbor(const size_t, sim_node *, const vm::node_val, const int, const int);
   void add_neighbor_count(const size_t, sim_node *, const size_t, const int);
   
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
		socket(NULL),
		slave(false)
   {
   }

	explicit sim_sched(vm::all *all, const vm::process_id id, sim_node *_node):
		sched::base(id, all),
		current_node(_node),
		socket(NULL),
		slave(true)
	{
	}
   
	virtual ~sim_sched(void);
};

}

#endif
