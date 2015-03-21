
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

static char* progname{nullptr};
static bool show_predicates{false};
static bool show_rules{false};

static void version(void) {
   cout << "Linear Meld program " << MAJOR_VERSION << "." << MINOR_VERSION
        << " (C++ " << __cplusplus << ")" << endl;
}

static void help(void) {
   version();
   cerr << progname << ": execute program" << endl;
   cerr << progname << " -c <scheduler> [options] -- arg1 arg2 ... argN"
        << endl;
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
   cerr << "\t-p \t\tshow predicates" << endl;
   cerr << "\t-x \t\tshow rules" << endl;

   exit(EXIT_SUCCESS);
}

static vm::machine_arguments read_arguments(int argc, char** argv) {
   vm::machine_arguments program_arguments;

   progname = *argv++;
   --argc;

   while (argc > 0 && (argv[0][0] == '-')) {
      switch (argv[0][1]) {
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
         case 'x':
            show_rules = true;
            break;
         case 'p':
            show_predicates = true;
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

int main(int argc, char** argv) {
   mem::ensure_pool();
   num_threads = 1;

   vm::machine_arguments margs(read_arguments(argc, argv));
   try {
      machine mac(num_threads, margs);

      if (show_predicates || show_rules) {
         vm::program& prog(*(vm::All->PROGRAM));
         if (show_predicates) prog.print_predicates(cout);
         if (show_rules) prog.print_rules(cout);
         exit(EXIT_SUCCESS);
      }

      run_program(mac);
   } catch (vm::load_file_error& err) {
      cerr << "File error: " << err.what() << endl;
      exit(EXIT_FAILURE);
   } catch (machine_error& err) {
      cerr << "VM error: " << err.what() << endl;
      exit(EXIT_FAILURE);
   } catch (db::database_error& err) {
      cerr << "Database error: " << err.what() << endl;
      exit(EXIT_FAILURE);
   }

   return EXIT_SUCCESS;
}
