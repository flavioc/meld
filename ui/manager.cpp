
#include "conf.hpp"

#ifdef USE_UI

#include <json_spirit.h>

#include "ui/manager.hpp"
#include "interface.hpp"
#include "version.hpp"
#include "db/node.hpp"
#include "db/database.hpp"
#include "vm/tuple.hpp"
#include "vm/program.hpp"
#include "ui/macros.hpp"
#include "vm/state.hpp"
#include "sched/serial_ui.hpp"

using namespace std;
using namespace websocketpp;
using namespace json_spirit;

namespace ui
{
	
manager *man;

manager::manager(void):
	next_set(0),
	done(false),
   all(NULL)
{
}

manager::~manager(void)
{
   for(client_list::iterator it(clients.begin()), end(clients.end()); it != end; it++)
      delete it->second;
}

#define ADD_FIELD(P1, P2) 								\
	UI_ADD_FIELD(main_obj, P1, P2)
#define DECLARE_JSON(TYPE) 							\
	Object main_obj; 										\
	ADD_FIELD("version", VERSION); 					\
	ADD_FIELD("msg", TYPE)
#define ADD_NODE_FIELD(NODE) 							\
	UI_ADD_NODE_FIELD(main_obj, NODE)
#define ADD_TUPLE_FIELD(TPL) 							\
	UI_ADD_TUPLE_FIELD(main_obj, TPL)

#define SEND_JSON(CONN) 								\
	(CONN)->send(write(main_obj))
#define SEND_CLIENT_JSON(CLIENT) 					\
	SEND_JSON((CLIENT)->conn)
	
#define SEND_ALL_CLIENTS() \
	for(client_list::iterator it(clients.begin()), end(clients.end()); it != end; it++) \
		SEND_CLIENT_JSON(it->second)
	
void
manager::on_open(connection_ptr conn)
{
   clients[conn] = new client(conn);

	DECLARE_JSON("init");

	ADD_FIELD("running", running ? Value(program_running) : Value());

	SEND_JSON(conn);
	
	if(running) {
		event_program(all->PROGRAM);
		event_database(all->DATABASE);
		// XXX: send all route facts
	}
}

void
manager::on_close(connection_ptr conn)
{
   clients.erase(conn);
}

static Value
get_field_from_obj(Object& obj, const string& field)
{
	for(Object::iterator it(obj.begin()), end(obj.end()); it != end; it++) {
		Pair& p(*it);

		if(p.name_ == field)
			return p.value_;
	}

	return UI_NIL;
}

static inline bool
is_obj(Value& v)
{
	return v.type() == obj_type;
}

static inline bool
is_string(Value& v)
{
	return v.type() == str_type;
}

static inline bool
is_int(Value& v)
{
	return v.type() == int_type;
}

void
manager::on_message(connection_ptr conn, message_ptr msg)
{
	const string content(msg->get_payload());

	Value ret;

	if(read(content, ret) && is_obj(ret)) {
		Object obj(ret.get_obj());
		Value type(get_field_from_obj(obj, "msg"));

		if(is_string(type)) {
			const string& str(type.get_str());
			
			if(str == "next")
				next_set++;
         else if(str == "jump") {
            Value node(get_field_from_obj(obj, "steps"));

            if(is_int(node)) {
               const int steps(node.get_int());
               next_set += steps;
            }
         } else if(str == "get_node") {
				if(!running)
					return;
					
				Value node(get_field_from_obj(obj, "node"));
				
				if(is_int(node)) {
					db::node::node_id id((db::node::node_id)node.get_int());

					DECLARE_JSON("node");
					
					ADD_FIELD("node", (int)id);
					ADD_FIELD("info", sched::serial_ui_local::dump_node(id));
					
					SEND_JSON(conn);
				}
			} else if(str == "change_node") {
				if(!running)
					return;
					
				Value node(get_field_from_obj(obj, "node"));
				
				if(is_int(node)) {
					db::node::node_id id((db::node::node_id)node.get_int());

					sched::serial_ui_local::change_node(id);
					
					DECLARE_JSON("changed_node");
					ADD_FIELD("node", (int)id);
					SEND_ALL_CLIENTS();
				}
			} else if(str == "terminate") {
				if(!done)
					done = true;
			} else {
				cerr << "Message not recognized: " << str << endl;
			}
		}
	}
}

void
manager::wait_for_next(void)
{
	if(!no_clients()) {
		while(next_set <= 0) {
			if(no_clients())
				return;
			usleep(100);
		}
		next_set--;
	}
}

void
manager::wait_for_done(void)
{
	if(!no_clients()) {
		while(!done) {
			if(no_clients())
				return;
			usleep(100);
		}
		done = false;
	}
}

void
manager::event_program_running(void)
{
	DECLARE_JSON("program_running");
		
	ADD_FIELD("running", program_running);
		
	SEND_ALL_CLIENTS();
}

void
manager::event_program_stopped(void)
{
	DECLARE_JSON("program_stopped");

	SEND_ALL_CLIENTS();
}

void
manager::event_database(db::database *db)
{
	DECLARE_JSON("database");

	ADD_FIELD("database", db->dump_json());
		
	SEND_ALL_CLIENTS();
}

void
manager::event_program(vm::program *pgrm)
{
	DECLARE_JSON("program");

	ADD_FIELD("program", pgrm->dump_json());

	Array rules;

   for(size_t i(0); i < pgrm->num_rules(); i++)
      UI_ADD_ELEM(rules, pgrm->get_rule(i)->get_string());

   ADD_FIELD("rules", rules);

	SEND_ALL_CLIENTS();
}

void
manager::event_persistent_derivation(const db::node *n, const vm::tuple *tpl)
{
	DECLARE_JSON("persistent_derivation");

	ADD_NODE_FIELD(n);
	ADD_TUPLE_FIELD(tpl);
	
	SEND_ALL_CLIENTS();
}

void
manager::event_linear_derivation(const db::node *n, const vm::tuple *tpl)
{
	DECLARE_JSON("linear_derivation");
	
	ADD_NODE_FIELD(n);
	ADD_TUPLE_FIELD(tpl);
	
	SEND_ALL_CLIENTS();
}

void
manager::event_tuple_send(const db::node *from, const db::node *to, const vm::tuple *tpl)
{
	DECLARE_JSON("tuple_send");
	
	ADD_FIELD("from", (int)(from->get_id()));
	ADD_FIELD("to", (int)(to->get_id()));
	ADD_TUPLE_FIELD(tpl);
	
	SEND_ALL_CLIENTS();
}

void
manager::event_linear_consumption(const db::node *node, const vm::tuple *tpl)
{
	DECLARE_JSON("linear_consumption");
	
	ADD_NODE_FIELD(node);
	ADD_TUPLE_FIELD(tpl);
	
	SEND_ALL_CLIENTS();
}

void
manager::event_step_done(const db::node *n)
{
	DECLARE_JSON("step_done");
	
	if(n)
		ADD_NODE_FIELD(n);
	
	SEND_ALL_CLIENTS();
}

void
manager::event_step_start(const db::node *n)
{
	DECLARE_JSON("step_start");
	
	ADD_NODE_FIELD(n);
	
	SEND_ALL_CLIENTS();
}

void
manager::event_program_termination(void)
{
	DECLARE_JSON("program_termination");
	
	SEND_ALL_CLIENTS();
}

void
manager::event_set_color(const db::node *n, const int r, const int g, const int b)
{
	DECLARE_JSON("set_color");
	
	ADD_NODE_FIELD(n);
	ADD_FIELD("r", r);
	ADD_FIELD("g", g);
	ADD_FIELD("b", b);
	
	SEND_ALL_CLIENTS();
}

void
manager::event_set_edge_label(const vm::node_val from, const vm::node_val to, const string& label)
{
	DECLARE_JSON("set_edge_label");
	
	ADD_FIELD("from", (int)from);
	ADD_FIELD("to", (int)to);
	ADD_FIELD("label", label);
	
	SEND_ALL_CLIENTS();
}

void
manager::event_rule_start(const db::node *who, const vm::rule *rule)
{
   DECLARE_JSON("rule_start");

   ADD_NODE_FIELD(who);
   ADD_FIELD("rule", (int)rule->get_id());

   SEND_ALL_CLIENTS();
}

void
manager::event_rule_applied(const db::node *who, const vm::rule *rule)
{
   DECLARE_JSON("rule_applied");

   ADD_NODE_FIELD(who);
   ADD_FIELD("rule", (int)rule->get_id());

   SEND_ALL_CLIENTS();
}

void
manager::event_new_node(const db::node *node)
{
   DECLARE_JSON("new_node");
   
   ADD_NODE_FIELD(node);
   ADD_FIELD("translated", (int)node->get_translated_id());

   SEND_ALL_CLIENTS();
}

}

#endif /* USE_UI */
