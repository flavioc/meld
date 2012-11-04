
#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include "conf.hpp"

#include <list>
#include <cstdlib>
#ifdef USE_UI
#include <websocketpp/websocketpp.hpp>
#endif
#include <map>

#include "vm/defs.hpp"
#include "ui/client.hpp"
#include "utils/atomic.hpp"

namespace db
{
class database;
class node;
}

namespace vm
{
class tuple;
class program;
}

namespace ui
{

#ifdef USE_UI	
typedef enum {
	EVENT_PROGRAM_START,
	EVENT_UNKNOWN
} event_type;

class manager: public websocketpp::server::handler
{
   private:

      typedef websocketpp::server::connection_ptr connection_ptr;
      typedef websocketpp::message::data_ptr message_ptr;

      typedef std::map<connection_ptr, client *> client_list;

      client_list clients;

		utils::atomic<int> next_set;
		volatile bool done;

   public:

      void on_open(connection_ptr);
      void on_close(connection_ptr);
      void on_message(connection_ptr, message_ptr);

		void wait_for_next(void);
		void wait_for_done(void);

		void event_program_running(void);
		void event_program_stopped(void);
		void event_database(db::database *);
		void event_program(vm::program *);
		void event_persistent_derivation(const db::node *, const vm::tuple *);
		void event_linear_derivation(const db::node *, const vm::tuple *);
		void event_tuple_send(const db::node *, const db::node *, const vm::tuple *);
		void event_linear_consumption(const db::node *, const vm::tuple *);
		void event_step_done(const db::node *);
		void event_step_start(const db::node *, const vm::tuple *);
		void event_program_termination(void);
		void event_set_color(const db::node *, const int, const int, const int);
		void event_set_edge_label(const vm::node_val, const vm::node_val, const std::string&);
      void event_rule_applied(const db::node *, const std::string&);
		
		bool no_clients(void) const { return num_clients() == 0; }
      size_t num_clients(void) const { return clients.size(); }

      explicit manager(void);
      virtual ~manager(void);
};

extern manager *man;
#endif

#ifdef USE_UI
#define LOG_RUN(PART)			if (ui::man) { ui::man->PART; }
#define LOG_PROGRAM_RUNNING() LOG_RUN(event_program_running())
#define LOG_PROGRAM_STOPPED() LOG_RUN(event_program_stopped())
#define LOG_DATABASE(DB)		LOG_RUN(event_database(DB))
#define LOG_PROGRAM(PRGM)		LOG_RUN(event_program(PRGM))
#define LOG_PERSISTENT_DERIVATION(NODE, TPL) LOG_RUN(event_persistent_derivation(NODE, TPL))
#define LOG_LINEAR_DERIVATION(NODE, TPL) LOG_RUN(event_linear_derivation(NODE, TPL))
#define LOG_TUPLE_SEND(FROM, TO, TPL) LOG_RUN(event_tuple_send(FROM, TO, TPL))
#define LOG_LINEAR_CONSUMPTION(NODE, TPL) LOG_RUN(event_linear_consumption(NODE, TPL))
#define LOG_STEP_DONE(NODE) LOG_RUN(event_step_done(NODE))
#define LOG_STEP_START(NODE, TPL) LOG_RUN(event_step_start(NODE, TPL))
#define LOG_PROGRAM_TERMINATION()	LOG_RUN(event_program_termination())
#define LOG_SET_COLOR(NODE, R, G, B)	LOG_RUN(event_set_color(NODE, R, G, B))
#define LOG_SET_EDGE_LABEL(FROM, TO, LABEL) LOG_RUN(event_set_edge_label(FROM, TO, LABEL))
#define LOG_RULE_APPLIED(NODE, RULE) LOG_RUN(event_rule_applied(NODE, RULE))
#define WAIT_FOR_NEXT()	LOG_RUN(wait_for_next())
#define WAIT_FOR_DONE() LOG_RUN(wait_for_done())
#else
#define LOG_PROGRAM_RUNNING()
#define LOG_PROGRAM_STOPPED()
#define LOG_DATABASE(DB)
#define LOG_PROGRAM(PRGM)
#define LOG_LINEAR_CONSUMPTION(NODE, TPL)
#define LOG_PERSISTENT_DERIVATION(NODE, TPL)
#define LOG_LINEAR_DERIVATION(NODE, TPL)
#define LOG_TUPLE_SEND(FROM, TO, TPL)
#define LOG_LINEAR_DERIVATION(NODE, TPL)
#define LOG_STEP_DONE(NODE)
#define LOG_STEP_START(NODE, TPL)
#define LOG_PROGRAM_TERMINATION()
#define LOG_SET_COLOR(NODE, R, G, B)
#define LOG_SET_EDGE_LABEL(FROM, TO, LABEL)
#define LOG_RULE_APPLIED(NODE, RULE)
#define WAIT_FOR_NEXT()
#define WAIT_FOR_DONE()
#endif

}

#endif

