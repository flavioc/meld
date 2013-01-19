
#ifndef SCHED_SERIAL_UI_HPP
#define SCHED_SERIAL_UI_HPP

#include "conf.hpp"

#include "sched/base.hpp"
#include "sched/nodes/serial.hpp"
#include "sched/serial.hpp"

#ifdef USE_UI
#include <json_spirit.h>
#endif

namespace sched
{
   
class serial_ui_local: public serial_local
{
private:
   
	bool first_done;
   bool stop;
	
public:

	virtual db::node* get_work(void);
	
	virtual void init(const size_t);
	virtual void end(void);
	
   static db::node* create_node(const db::node::node_id id, const db::node::node_id trans, vm::all *all)
   {
      return dynamic_cast<db::node*>(new sched::serial_node(id, trans, all));
   }

#ifdef USE_UI
	json_spirit::Value dump_this_node(const db::node::node_id);
	static json_spirit::Value dump_node(const db::node::node_id);
#endif

	void change_to_node(serial_node*);
	static void change_node(const db::node::node_id);

   serial_ui_local *find_scheduler(const db::node *) { return this; }
   
	explicit serial_ui_local(vm::all *);
   
   virtual ~serial_ui_local(void) {}
};

}

#endif
