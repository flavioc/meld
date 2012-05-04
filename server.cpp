
#include <iostream>
#include <string>
#include <vector>
#include <readline/readline.h>
#include <boost/algorithm/string.hpp>
#include <pion/net/HTTPServer.hpp>
#include <pion/net/HTTPTypes.hpp>
#include <pion/net/HTTPRequest.hpp>
#include <pion/net/HTTPResponseWriter.hpp>

#include "process/machine.hpp"
#include "utils/utils.hpp"
#include "process/router.hpp"
#include "interface.hpp"
#include "version.hpp"
#include "utils/atomic.hpp"

using namespace utils;
using namespace process;
using namespace std;
using namespace sched;
using namespace boost;
using namespace pion;
using namespace pion::net;
using namespace utils;

static char *progname = NULL;
static int port = 0;
static atomic<int> num_users(0);

static void
help(void)
{
	cerr << "server: execute meld server" << endl;
	cerr << "\t\t-p \t\tset server port" << endl;
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
	cout << "\tversion\t show version" << endl;
	cout << "\thelp\t show this text" << endl;
	cout << "\texit\t exit the server" << endl;
}

static void
show_version(void)
{
	cout << "Meld Parallel Environment " << VERSION << endl;
	cout << "using ";
	switch(sched_type) {
		case SCHED_SERIAL_LOCAL:
			cout << "sequential scheduler";
			break;
		default:
			cout << "other scheduler";
			break;
	}
	cout << endl;
}

typedef enum {
	COMMAND_EXIT,
	COMMAND_LOAD,
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
		
	split(command_args, cmd, is_any_of(" \t\n"), token_compress_on);
	
	if(!command_args.empty()) {
		if(command_args[0] == "load") {
			if(command_args.size() == 2) {
				return COMMAND_LOAD;
			}
		}
	}
	return COMMAND_NONE;
}

void
handleRoot(HTTPRequestPtr& request, TCPConnectionPtr& tcp_conn)
{
	num_users++;
	
	HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *request,
	                                             boost::bind(&TCPConnection::finish, tcp_conn)));
	
	writer->getResponse().setContentType(HTTPTypes::CONTENT_TYPE_TEXT);
	writer->getResponse().setStatusCode(HTTPTypes::RESPONSE_CODE_OK);
	writer->getResponse().setStatusMessage(HTTPTypes::RESPONSE_MESSAGE_OK);
	
	writer->write(string("<html><body>Hello World!</body></html>"));
	
	writer->send();
	
	sleep(10);
	
	num_users--;
}

static void
show_status(void)
{
	show_version();
	cout << "Port: " << port << endl;
	cout << "# Users: " << num_users << endl;
}

int
main(int argc, char **argv)
{
   read_arguments(argc, argv);

	if(sched_type == SCHED_UNKNOWN) {
		cerr << "Error: please provide a scheduler" << endl;
		return EXIT_FAILURE;
	}
	
	show_version();
	
	const int readline_ret(rl_initialize());
	
	if(readline_ret != 0) {
		cerr << "Failure to initialize readline" << endl;
		return EXIT_FAILURE;
	}
	
	HTTPServerPtr server(new HTTPServer(port));
	server->addResource("/", &handleRoot);
	server->start();
	
	while (true) {
		char *input(readline("> "));
		
		if(input == NULL)
			continue;
		
		command_type cmd(parse_command(string(input)));

      delete input;
		
		switch(cmd) {
			case COMMAND_EXIT: return EXIT_SUCCESS;
			case COMMAND_VERSION: show_version(); break;
			case COMMAND_HELP: show_inter_help(); break;
			case COMMAND_STATUS: show_status(); break;
			case COMMAND_NONE: cout << "Command not recognized" << endl; break;
			case COMMAND_LOAD: {
				const string filename(command_args[1]);
				
				run_program(argc, argv, filename.c_str());
			}
			break;
			default: break;
		}
	}

   return EXIT_SUCCESS;
}
