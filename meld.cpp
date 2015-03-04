
#include <iostream>
#include <vector>

#include "machine.hpp"
#include "utils/utils.hpp"
#include "utils/fs.hpp"
#include "vm/state.hpp"
#include "vm/exec.hpp"
#include "interface.hpp"
#include "version.hpp"

using namespace utils;
using namespace process;
using namespace std;
using namespace sched;

static char *program = NULL;
static char *data_file = NULL;
static char *progname = NULL;

static void version(void) {
   cout << "Linear Meld interpreter " << MAJOR_VERSION << "." << MINOR_VERSION
        << " (C++ " << __cplusplus << ")" << endl;
}

static void help(void) {
   version();
   cerr << "meld: execute meld program" << endl;
   cerr << "meld -f <program file> -c <scheduler> [options] -- arg1 arg2 ... "
           "argN" << endl;
   cerr << "\t-f <name>\tmeld program" << endl;
   cerr << "\t-r <data file>\tdata file for meld program" << endl;
   help_schedulers();
   cerr << "\t-n \t\tno dynamic scheduling" << endl;
   cerr << "\t-w \t\tdisable work stealing" << endl;
   cerr << "\t-t \t\ttime execution" << endl;
#ifdef INSTRUMENTATION
   cerr << "\t-i <file>\tdump time statistics" << endl;
#endif
   cerr << "\t-s \t\tshows database" << endl;
   cerr << "\t-d \t\tdump database (debug option)" << endl;
   cerr << "\t-h \t\tshow this screen" << endl;

   exit(EXIT_SUCCESS);
}

static vm::machine_arguments read_arguments(int argc, char **argv) {
   vm::machine_arguments program_arguments;

   progname = *argv++;
   --argc;

   while (argc > 0 && (argv[0][0] == '-')) {
      switch (argv[0][1]) {
         case 'f': {
            if (program != NULL || argc < 2) help();

            program = argv[1];

            argc--;
            argv++;
         } break;
         case 'r': {
            if (data_file != NULL || argc < 2) help();

            data_file = argv[1];

            argc--;
            argv++;
         } break;
         case 'c': {
            if (argc < 2) help();
            parse_sched(argv[1]);
            argc--;
            argv++;
         } break;
         case 's':
            show_database = true;
            break;
         case 'd':
            dump_database = true;
            break;
         case 't':
            time_execution = true;
            break;
#ifdef INSTRUMENTATION
         case 'i':
            if (argc < 2) help();

            statistics::set_stat_file(string(argv[1]));
            argc--;
            argv++;
            break;
#endif
         case 'n':
            scheduling_mechanism = false;
            break;
         case 'w':
            work_stealing = false;
            break;
         case 'h':
            help();
            break;
         case '-':

            for (--argc, ++argv; argc > 0; --argc, ++argv)
               program_arguments.push_back(string(*argv));
            break;
         default:
            help();
      }

      /* advance */
      argc--;
      argv++;
   }

   return program_arguments;
}

int main(int argc, char **argv) {
   num_threads = 1;

   vm::machine_arguments margs(read_arguments(argc, argv));

   if (program == NULL) {
      version();
      cerr << "Error: please provide a byte-code file to run..." << endl;
      return EXIT_FAILURE;
   }

   if (!file_exists(program)) {
      cerr << "Error: file " << program << " does not exist or is not readable"
           << endl;
      return EXIT_FAILURE;
   }

   try {
      machine mac(program, num_threads, margs,
                  data_file == NULL ? string("") : string(data_file));
      run_program(mac);
   } catch (vm::load_file_error &err) {
      cerr << "File error: " << err.what() << endl;
      exit(EXIT_FAILURE);
   } catch (vm::vm_exec_error &err) {
      cerr << "Internal error: " << err.what() << endl;
      exit(EXIT_FAILURE);
   } catch (machine_error &err) {
      cerr << "VM error: " << err.what() << endl;
      exit(EXIT_FAILURE);
   } catch (db::database_error &err) {
      cerr << "Database error: " << err.what() << endl;
      exit(EXIT_FAILURE);
   }

   return EXIT_SUCCESS;
}
