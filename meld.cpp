
#include <iostream>

#include "process/machine.hpp"
#include "utils/utils.hpp"
#include "process/router.hpp"

#include "interface.hpp"

using namespace utils;
using namespace process;
using namespace std;
using namespace sched;

static char *program = NULL;
static char *progname = NULL;

static void
help(void)
{
	cerr << "meld: execute meld program" << endl;
	cerr << "\t-f <name>\tmeld program" << endl;
	help_schedulers();
	cerr << "\t-t \t\ttime execution" << endl;
	cerr << "\t-m \t\tmemory statistics" << endl;
	cerr << "\t-i <file>\tdump time statistics" << endl;
	cerr << "\t-s \t\tshows database" << endl;
   cerr << "\t-d \t\tdump database (debug option)" << endl;
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
         case 'f': {
            if (program != NULL || argc < 2)
               help();

            program = argv[1];

            argc--;
            argv++;
         }
         break;
         case 'c': {
            if (sched_type != SCHED_UNKNOWN)
               help();
            
            parse_sched(argv[1]);
            argc--;
            argv++;
         }
         break;
         case 's':
            show_database = true;
            break;
         case 'd':
            dump_database = true;
            break;
         case 't':
            time_execution = true;
            break;
         case 'm':
            memory_statistics = true;
            break;
         case 'i':
            if(argc < 2)
               help();
               
            statistics::set_stat_file(string(argv[1]));
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
}

int
main(int argc, char **argv)
{
   read_arguments(argc, argv);

   if(program == NULL && sched_type == SCHED_UNKNOWN) {
      cerr << "Error: please provide scheduler type and a program to run" << endl;
      return EXIT_FAILURE;
   } else if(program == NULL && sched_type != SCHED_UNKNOWN) {
		cerr << "Error: please provide a program to run" << endl;
      return EXIT_FAILURE;
   } else if(program != NULL && sched_type == SCHED_UNKNOWN) {
		cerr << "Error: please pick a scheduler to use" << endl;
      return EXIT_FAILURE;
   }
   
	run_program(argc, argv, program);

   return EXIT_SUCCESS;
}
