
#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include <list>
#include <cstdlib>
#include <websocketpp/websocketpp.hpp>
#include <map>

#include "conf.hpp"
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

   public:

      void on_open(connection_ptr);
      void on_close(connection_ptr);
      void on_message(connection_ptr, message_ptr);

		void wait_for_next(void);

		void event_program_running(void);
		void event_program_stopped(void);
		void event_database(db::database *);
		void event_program(vm::program *);
		void event_persistent_derivation(db::node *, vm::tuple *);

      size_t num_clients(void) const { return clients.size(); }

      explicit manager(void);
      virtual ~manager(void);
};

extern manager *man;

#ifdef USE_UI
#define LOG_RUN(PART)			if (ui::man) { ui::man->PART; }
#define LOG_PROGRAM_RUNNING() LOG_RUN(event_program_running())
#define LOG_PROGRAM_STOPPED() LOG_RUN(event_program_stopped())
#define LOG_DATABASE(DB)		LOG_RUN(event_database(DB))
#define LOG_PROGRAM(PRGM)		LOG_RUN(event_program(PRGM))
#define LOG_PERSISTENT_DERIVATION(NODE, TPL) LOG_RUN(event_persistent_derivation(NODE, TPL))
#define WAIT_FOR_NEXT()			LOG_RUN(wait_for_next())
#else
#define LOG_PROGRAM_RUNNING()
#define LOG_PROGRAM_STOPPED()
#define LOG_DATABASE(DB)
#define LOG_PERSISTENT_DERIVATION(NODE, TPL)
#define LOG_PROGRAM(PRGM)
#define WAIT_FOR_NEXT()
#endif

}

#endif

