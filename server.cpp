
#include "conf.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <exception>
#ifdef USE_UI
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>

#include "process/machine.hpp"
#include "utils/utils.hpp"
#include "process/router.hpp"
#include "interface.hpp"
#include "version.hpp"
#include "utils/atomic.hpp"
#include "ui/client.hpp"
#include "ui/manager.hpp"

using namespace utils;
using namespace process;
using namespace std;
using namespace sched;
using namespace boost;
using namespace utils;
#ifdef USE_UI
using namespace websocketpp;
using namespace ui;
#endif

static char *progname = NULL;
static int port = 0;

static void
help(void)
{
	cerr << "server: execute meld server" << endl;
	cerr << "\t-p \t\tset server port" << endl;
	help_schedulers();
   cerr << "\t-h \t\tshow this screen" << endl;

   exit(EXIT_SUCCESS);
}

static void
read_arguments(int argc, char **argv)
{
   progname = *argv++;
   --argc;

   while (argc > 0 && (argv[0][0] == '-')) {
      switch(argv[0][1]) {
         case 'c': {
            if (sched_type != SCHED_UNKNOWN)
               help();
            
            parse_sched(argv[1]);
            argc--;
            argv++;
         }
         break;
			case 'p':
				if(argc < 2)
					help();
				port = atoi(argv[1]);
				argc--;
				argv++;
				break;
         case 'h':
            help();
            break;
         default:
            help();
      }

      /* advance */
      argc--; argv++;
   }

	if(port == 0) {
		cerr << "Error: no server port set" << endl;
		exit(EXIT_FAILURE);
	}
}

static void
show_inter_help(void)
{
	cout << "Meld server command line" << endl;
	
	cout << "Commands:" << endl;
	cout << "\tstatus\t\t show server status" << endl;
	cout << "\tversion\t\t show version" << endl;
	cout << "\thelp\t\t show this text" << endl;
	cout << "\texit\t\t exit the server" << endl;
}

static void
show_version(void)
{
	cout << "Meld Parallel Environment " << MAJOR_VERSION << "." << MINOR_VERSION << endl;
	cout << "using ";
	switch(sched_type) {
		case SCHED_SERIAL_UI:
			cout << "ui sequential scheduler";
			break;
		default:
			cout << "other scheduler";
			break;
	}
	cout << endl;
}

typedef enum {
	COMMAND_EXIT,
	COMMAND_VERSION,
	COMMAND_HELP,
	COMMAND_STATUS,
	COMMAND_NONE
} command_type;

static vector<string> command_args;

static command_type
parse_command(string cmd)
{
	trim(cmd);
	
	if(cmd.empty())
		return COMMAND_NONE;
		
	if(cmd == "exit")
		return COMMAND_EXIT;
	if(cmd == "version")
		return COMMAND_VERSION;
	if(cmd == "help")
		return COMMAND_HELP;
	if(cmd == "status")
		return COMMAND_STATUS;
   if(cmd == "quit")
      return COMMAND_EXIT;
		
#if 0
	split(command_args, cmd, is_any_of(" \t\n"), token_compress_on);
	
	if(!command_args.empty()) {
		if(command_args[0] == "load") {
			if(command_args.size() >= 2) {
				return COMMAND_LOAD;
			}
		}
	}
#endif
	return COMMAND_NONE;
}

#ifdef USE_UI
static void
show_status(void)
{
	show_version();
	cout << "Port: " << port << endl;
	cout << "# Users: " << man->num_clients() << endl;
   man->print_clients();
}

static void
run_server(void)
{
   server::handler::ptr h(man);
   server endpoint(h);

   endpoint.listen(port);
}
#endif

int
main(int argc, char **argv)
{
#ifdef USE_UI
   read_arguments(argc, argv);

	if(sched_type == SCHED_UNKNOWN) {
		cerr << "Error: please provide a scheduler" << endl;
		return EXIT_FAILURE;
	}
	
	const int readline_ret(rl_initialize());
	
	if(readline_ret != 0) {
		cerr << "Failure to initialize readline" << endl;
		return EXIT_FAILURE;
	}
	
	try {
      man = new ui::manager();

		show_version();

      thread aux(run_server);
			
		while (true) {
			char *input(readline("\n>> "));
		
			if(input == NULL)
				continue;
		
			command_type cmd(parse_command(string(input)));

      	add_history(input);
      	delete input;

			switch(cmd) {
				case COMMAND_EXIT: return EXIT_SUCCESS;
				case COMMAND_VERSION: show_version(); break;
				case COMMAND_HELP: show_inter_help(); break;
				case COMMAND_STATUS: show_status(); break;
				case COMMAND_NONE: cout << "Command not recognized" << endl; break;
				default: break;
			}
		}
	} catch(std::exception& e) {
		cerr << "Failed to initiate server: " << e.what() << endl;
		return EXIT_FAILURE;
	}

   return EXIT_SUCCESS;
#else
	cerr << "No UI was compiled" << endl;
	return EXIT_FAILURE;
#endif
}
