
#ifndef SCHED_LOCAL_SERIAL_UI_HPP
#define SCHED_LOCAL_SERIAL_UI_HPP

#include "conf.hpp"

#include "sched/base.hpp"
#include "sched/nodes/serial.hpp"
#include "sched/local/serial.hpp"

#ifdef USE_UI
#include <json_spirit.h>
#endif

namespace sched
{
   
class serial_ui_local: public serial_local
{
private:
   
	bool first_done;
	
public:

	virtual void new_persistent_derivation(db::node *, vm::tuple *);
	virtual void new_linear_derivation(db::node *, vm::tuple *);
	virtual void new_linear_consumption(db::node *, vm::tuple *);
	
	virtual void new_work(const db::node *, process::work&);
	virtual bool get_work(process::work&);
	
	virtual void init(const size_t);
	virtual void end(void);
	
	virtual void set_node_color(db::node *, const int, const int, const int);
	virtual void set_edge_label(db::node *, const db::node::node_id, const runtime::rstring::ptr);
   
   static db::node* create_node(const db::node::node_id id, const db::node::node_id trans)
   {
      return dynamic_cast<db::node*>(new sched::serial_node(id, trans));
   }

#ifdef USE_UI
	json_spirit::Value dump_this_node(const db::node::node_id);
	static json_spirit::Value dump_node(const db::node::node_id);
#endif

	void change_to_node(serial_node*);
	static void change_node(const db::node::node_id);

   serial_ui_local *find_scheduler(const db::node *) { return this; }
   
	explicit serial_ui_local(void);
   
   virtual ~serial_ui_local(void) {}
};

}

#endif
