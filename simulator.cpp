
#include <cstdlib>

#include "conf.hpp"
#include "process/machine.hpp"
#include "utils/utils.hpp"
#include "process/router.hpp"
#include "interface.hpp"
#include "version.hpp"
#include "utils/atomic.hpp"
#include "utils/fs.hpp"
#include "ui/client.hpp"
#include "ui/manager.hpp"
#include "sched/sim.hpp"

using namespace utils;
using namespace process;
using namespace std;
using namespace sched;
using namespace boost;
using namespace utils;

#ifdef USE_SIM
static char *progname = NULL;
static char *meldprog = NULL;
static int port = 0;

static void
help(void)
{
	cerr << "simulator: execute meld programs on a simulator" << endl;
	cerr << "\t-p \t\tset server port" << endl;
	cerr << "\t-f \t\tmeldprogram" << endl;
   cerr << "\t-h \t\tshow this screen" << endl;

   exit(EXIT_SUCCESS);
}

static vm::machine_arguments
read_arguments(int argc, char **argv)
{
	vm::machine_arguments program_arguments;
	
   progname = *argv++;
   --argc;

   while (argc > 0 && (argv[0][0] == '-')) {
      switch(argv[0][1]) {
         case 'p':
				if(argc < 2)
					help();
				port = atoi(argv[1]);
				argc--;
				argv++;
				break;
			case 'f':
				if(argc < 2)
					help();
				meldprog = argv[1];
				argc--;
				argv++;
				break;
         case 'h':
            help();
            break;
			case '-':

				for(--argc, ++argv ; argc > 0; --argc, ++argv)
					program_arguments.push_back(string(*argv));
						
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
	
	return program_arguments;
}
#endif

int
main(int argc, char **argv)
{
#ifdef USE_SIM
	vm::machine_arguments margs(read_arguments(argc, argv));
	
	if(meldprog == NULL) {
		cerr << "No program in input" << endl;
		return EXIT_FAILURE;
	}
	
	if(!file_exists(meldprog)) {
		cerr << "Meld program " << meldprog << " does not exist" << endl;
		return EXIT_FAILURE;
	}
	
	// setup scheduler
	sched_type = SCHED_SIM;
	num_threads = 1;
	sched::sim_sched::PORT = port;
	show_database = true;
	
	try {
      run_program(argc, argv, meldprog, margs, NULL);
   } catch(machine_error& err) {
      cerr << "VM error: " << err.what() << endl;
      exit(EXIT_FAILURE);
   } catch(db::database_error& err) {
      cerr << "Database error: " << err.what() << endl;
      exit(EXIT_FAILURE);
   }
	
	return EXIT_SUCCESS;
#else
   return EXIT_FAILURE;
#endif
}
