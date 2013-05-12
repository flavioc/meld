
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
#include "process/machine.hpp"
#include "utils/fs.hpp"

using namespace std;
using namespace websocketpp;
using namespace json_spirit;
using namespace process;
using namespace sched;
using namespace utils;

namespace ui
{
	
manager *man;

static pthread_key_t client_key;

static void
cleanup_keys(void)
{
   pthread_key_delete(client_key);
}

static bool
init(void)
{
   int ret(pthread_key_create(&client_key, NULL));
   assert(ret == 0);
   atexit(cleanup_keys);
   return true;
}

static bool started(init());

void
manager::print_clients(void) const
{
   cout << "# Users served: " << clients_served << endl;
   client_mtx.lock();
   
   for(client_list::const_iterator it(clients.begin()), end(clients.end());
         it != end;
         ++it)
   {
      const connection_ptr ptr(it->first);
      client *cl(it->second);
      cout << "> ";
      cout << cl->conn << " ";
      if(cl->all != NULL) {
         cout << cl->all->PROGRAM->get_name();
      }
      cout << " (units: " << cl->cunits << ")";
      cout << endl;
   }

   client_mtx.unlock();
}

void
manager::cleanup(void)
{
   for(vector<client*>::iterator it(cleanup_list.begin()); it != cleanup_list.end(); ++it) {
      client *cl(*it);
      if(cl->th != NULL) {
         cl->th->join();
         delete cl->th;
      }
      delete cl;
   }
   cleanup_list.clear();
}

manager::manager(void):
   clients_served(0), all(NULL)
{
}

manager::~manager(void)
{
   for(client_list::iterator it(clients.begin()), end(clients.end()); it != end; it++)
      delete it->second;
   cleanup();
}

#define ADD_FIELD(P1, P2) 								\
	UI_ADD_FIELD(main_obj, P1, P2)
#define DECLARE_JSON(TYPE) 							\
	Object main_obj; 										\
	ADD_FIELD("version", MAJOR_VERSION); 			\
	ADD_FIELD("msg", TYPE)
#define ADD_NODE_FIELD(NODE) 							\
	UI_ADD_NODE_FIELD(main_obj, NODE)
#define ADD_TUPLE_FIELD(TPL) 							\
	UI_ADD_TUPLE_FIELD(main_obj, TPL)

#define SEND_JSON(CONN) 								\
	(CONN)->send(write(main_obj))
#define SEND_CLIENT_JSON(CLIENT) 					\
	SEND_JSON((CLIENT)->conn)
	
#if 0
#define SEND_ALL_CLIENTS() \
	for(client_list::iterator it(clients.begin()), end(clients.end()); it != end; it++) \
		SEND_CLIENT_JSON(it->second)
#endif
#define SEND_CURRENT_CLIENT()  do {                                        \
   client *cl((client*)pthread_getspecific(client_key));                   \
   SEND_CLIENT_JSON(cl);                                                   \
} while(false)
	
void
manager::on_open(connection_ptr conn)
{
   clients_served++;
   client_mtx.lock();
   clients[conn] = new client(conn);
   client_mtx.unlock();
}

void
manager::on_close(connection_ptr conn)
{
   client *cl(get_client(conn));

   cl->done = true;

   client_mtx.lock();
   clients.erase(conn);
   client_mtx.unlock();

   cleanup_list.push_back(cl);
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

static inline bool is_not_alnum(char c)
{
   return !isalnum(c);
}

bool file_is_valid(const std::string &str)
{
   return find_if(str.begin(), str.end(), is_not_alnum) == str.end();
}

static bool
valid_temp_file(string& file)
{
   if(file_is_valid(file)) {
      const string complete_name(string(TEMPORARY_CODE_DIRECTORY) + "/" + file + ".m");
      if(file_exists(complete_name)) {
         file = complete_name;
         return true;
      }
   }

   return false;
}

static void
delete_temp_file(const string& file)
{
   const int ret(unlink(file.c_str()));

   (void)ret;
}

void
manager::start_file(const string& file, client *cl, bool temp)
{
   pthread_setspecific(client_key, cl);

   scheduler_type sched_type(SCHED_SERIAL_UI);
   try {
      router rout(1, 0, NULL, is_mpi_sched(sched_type));
      machine mac(file, rout, 1, sched_type);

      cl->all = mac.get_all();

      LOG_PROGRAM_RUNNING();

      if(temp) {
         delete_temp_file(file);
      }

      mac.start();

      LOG_PROGRAM_STOPPED();
   } catch(std::exception& e) {
      cerr << e.what() << endl;
   }

   client_mtx.lock();
   cl->all = NULL;
   client_mtx.unlock();
}

client*
manager::get_client(connection_ptr conn)
{
   client_list::iterator it(clients.find(conn));
   assert(it != clients.end());
   client *cl(it->second);
   return cl;
}

void
manager::on_message(connection_ptr conn, message_ptr msg)
{
	const string content(msg->get_payload());

	Value ret;

   cleanup(); // do some maintenance

	if(read(content, ret) && is_obj(ret)) {
		Object obj(ret.get_obj());
		Value type(get_field_from_obj(obj, "msg"));

		if(is_string(type)) {
			const string& str(type.get_str());
			
			if(str == "next") {
            client *cl(get_client(conn));
            cl->counter++;
         } else if(str == "load_temp") {

            Value filev(get_field_from_obj(obj, "file"));
            string file(filev.get_str());

            if(valid_temp_file(file)) {
               client *cl(get_client(conn));
               cl->done = false;
               cl->counter = 0;

               if(cl->th != NULL) {
                  delete cl->th;
                  cl->th = NULL;
               }

               boost::thread *th = new boost::thread(boost::bind(&ui::manager::start_file, this, file, cl, true));

               cl->th = th;
            }
         } else if(str == "load" || str == "load_temp") {
            Value filev(get_field_from_obj(obj, "file"));
            string file(filev.get_str());
            client *cl(get_client(conn));
            cl->done = false;
            cl->counter = 0;

            if(cl->th != NULL) {
               delete cl->th;
               cl->th = NULL;
            }

            boost::thread *th = new boost::thread(boost::bind(&ui::manager::start_file, this, file, cl, false));

            cl->th = th;

         } else if(str == "jump") {
            Value node(get_field_from_obj(obj, "steps"));

            if(is_int(node)) {
               const int steps(node.get_int());
               client *cl(get_client(conn));

               cl->counter += steps;
            }
         } else if(str == "get_node") {
				Value node(get_field_from_obj(obj, "node"));
				
				if(is_int(node)) {
					db::node::node_id id((db::node::node_id)node.get_int());

					DECLARE_JSON("node");
					
					ADD_FIELD("node", (int)id);
					ADD_FIELD("info", sched::serial_ui_local::dump_node(id));
					
					SEND_JSON(conn);
				}
			} else if(str == "change_node") {
				Value node(get_field_from_obj(obj, "node"));
				
				if(is_int(node)) {
					db::node::node_id id((db::node::node_id)node.get_int());

					sched::serial_ui_local::change_node(id);
					
					DECLARE_JSON("changed_node");
					ADD_FIELD("node", (int)id);
					SEND_JSON(conn);
				}
			} else if(str == "terminate") {
            client* cl(get_client(conn));
            cl->done = true;
			} else {
				cerr << "Message not recognized: " << str << endl;
			}
		}
	}
}

bool
manager::wait_for_next(void)
{
   client *cl((client*)pthread_getspecific(client_key));
   assert(cl != NULL);

   cl->cunits++;

   while(cl->counter <= 0) {
      if(cl->done)
         return true;
      usleep(100);
   }
   if(cl->done)
      return true;
   cl->counter--;
   return true;
}

void
manager::wait_for_done(void)
{
   client *cl((client*)pthread_getspecific(client_key));
   assert(cl != NULL);

   while(!cl->done) {
      usleep(100);
   }
}

void
manager::event_program_running(void)
{
   client *cl((client*)pthread_getspecific(client_key));

	DECLARE_JSON("program_running");
		
	ADD_FIELD("running", cl->all->PROGRAM->get_name());
		
   SEND_CURRENT_CLIENT();
}

void
manager::event_program_stopped(void)
{
	DECLARE_JSON("program_stopped");

   SEND_CURRENT_CLIENT();
}

void
manager::event_database(db::database *db)
{
	DECLARE_JSON("database");

	ADD_FIELD("database", db->dump_json());
		
   SEND_CURRENT_CLIENT();
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

   SEND_CURRENT_CLIENT();
}

void
manager::event_persistent_derivation(const db::node *n, const vm::tuple *tpl)
{
	DECLARE_JSON("persistent_derivation");

	ADD_NODE_FIELD(n);
	ADD_TUPLE_FIELD(tpl);
	
   SEND_CURRENT_CLIENT();
}

void
manager::event_linear_derivation(const db::node *n, const vm::tuple *tpl)
{
	DECLARE_JSON("linear_derivation");
	
	ADD_NODE_FIELD(n);
	ADD_TUPLE_FIELD(tpl);
	
   SEND_CURRENT_CLIENT();
}

void
manager::event_tuple_send(const db::node *from, const db::node *to, const vm::tuple *tpl)
{
	DECLARE_JSON("tuple_send");
	
	ADD_FIELD("from", (int)(from->get_id()));
	ADD_FIELD("to", (int)(to->get_id()));
	ADD_TUPLE_FIELD(tpl);
	
   SEND_CURRENT_CLIENT();
}

void
manager::event_linear_consumption(const db::node *node, const vm::tuple *tpl)
{
	DECLARE_JSON("linear_consumption");
	
	ADD_NODE_FIELD(node);
	ADD_TUPLE_FIELD(tpl);
	
   SEND_CURRENT_CLIENT();
}

void
manager::event_step_done(const db::node *n)
{
	DECLARE_JSON("step_done");
	
	if(n)
		ADD_NODE_FIELD(n);
	
   SEND_CURRENT_CLIENT();
}

void
manager::event_step_start(const db::node *n)
{
	DECLARE_JSON("step_start");
	
	ADD_NODE_FIELD(n);
	
   SEND_CURRENT_CLIENT();
}

void
manager::event_program_termination(void)
{
	DECLARE_JSON("program_termination");
	
   SEND_CURRENT_CLIENT();
}

void
manager::event_set_color(const db::node *n, const int r, const int g, const int b)
{
	DECLARE_JSON("set_color");
	
	ADD_NODE_FIELD(n);
	ADD_FIELD("r", r);
	ADD_FIELD("g", g);
	ADD_FIELD("b", b);
	
   SEND_CURRENT_CLIENT();
}

void
manager::event_set_edge_label(const vm::node_val from, const vm::node_val to, const string& label)
{
	DECLARE_JSON("set_edge_label");
	
	ADD_FIELD("from", (int)from);
	ADD_FIELD("to", (int)to);
	ADD_FIELD("label", label);
	
   SEND_CURRENT_CLIENT();
}

void
manager::event_rule_start(const db::node *who, const vm::rule *rule)
{
   DECLARE_JSON("rule_start");

   ADD_NODE_FIELD(who);
   ADD_FIELD("rule", (int)rule->get_id());

   SEND_CURRENT_CLIENT();
}

void
manager::event_rule_applied(const db::node *who, const vm::rule *rule)
{
   DECLARE_JSON("rule_applied");

   ADD_NODE_FIELD(who);
   ADD_FIELD("rule", (int)rule->get_id());

   SEND_CURRENT_CLIENT();
}

void
manager::event_new_node(const db::node *node)
{
   DECLARE_JSON("new_node");
   
   ADD_NODE_FIELD(node);
   ADD_FIELD("translated", (int)node->get_translated_id());

   SEND_CURRENT_CLIENT();
}

}

#endif /* USE_UI */
