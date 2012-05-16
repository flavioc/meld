
#include <json_spirit.h>

#include "ui/manager.hpp"
#include "interface.hpp"
#include "version.hpp"
#include "db/node.hpp"
#include "db/database.hpp"
#include "vm/tuple.hpp"
#include "vm/program.hpp"
#include "ui/macros.hpp"

using namespace std;
using namespace websocketpp;
using namespace json_spirit;

namespace ui
{
	
manager *man;

manager::manager(void):
	next_set(1)
{
}

manager::~manager(void)
{
   for(client_list::iterator it(clients.begin()), end(clients.end()); it != end; it++)
      delete it->second;
}

#define ADD_FIELD(P1, P2) \
	UI_ADD_FIELD(main_obj, P1, P2)
#define DECLARE_JSON(TYPE) \
	Object main_obj; \
	ADD_FIELD("version", VERSION); \
	ADD_FIELD("msg", TYPE)

#define SEND_JSON(CONN) \
	(CONN)->send(write(main_obj))
#define SEND_CLIENT_JSON(CLIENT) \
	SEND_JSON((CLIENT)->conn)
	
void
manager::on_open(connection_ptr conn)
{
   clients[conn] = new client(conn);

	DECLARE_JSON("init");

	ADD_FIELD("running", running ? Value(program_running) : Value());

	SEND_JSON(conn);
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

void
manager::on_message(connection_ptr conn, message_ptr msg)
{
	const string content(msg->get_payload());

	Value ret;

	if(read(content, ret) && is_obj(ret)) {
		Object obj(ret.get_obj());
		Value type(get_field_from_obj(obj, "msg"));

		if(is_string(type)) {
			next_set--;
		}
	}
}

void
manager::wait_for_next(void)
{
	while(next_set > 0) {
		usleep(100);
	}
	next_set++;
}

#define SEND_ALL_CLIENTS() \
	for(client_list::iterator it(clients.begin()), end(clients.end()); it != end; it++) \
		SEND_CLIENT_JSON(it->second)

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

	SEND_ALL_CLIENTS();
}

void
manager::event_persistent_derivation(db::node *n, vm::tuple *tpl)
{
	DECLARE_JSON("persistent_derivation");

	ADD_FIELD("node", (int)n->get_id());

	Value tpljson(tpl->dump_json());

	ADD_FIELD("tuple", tpljson);

	SEND_ALL_CLIENTS();
}

}
