
#include <iostream>

#include "process/machine.hpp"
#include "utils/utils.hpp"
#include "process/router.hpp"
#include "stat/stat.hpp"

using namespace utils;
using namespace process;
using namespace std;
using namespace sched;

static size_t num_threads = 0;
static char *program = NULL;
static char *progname = NULL;
static scheduler_type sched_type = SCHED_UNKNOWN;
static bool show_database = false;
static bool dump_database = false;
static bool time_execution = false;
static bool memory_statistics = false;

static void
help(void)
{
   fprintf(stderr, "meld: execute meld program\n");
   fprintf(stderr, "\t-f <name>\tmeld program\n");
   fprintf(stderr, "\t-c <scheduler>\tselect scheduling type\n");
   fprintf(stderr, "\t\t\tsl simple serial scheduler\n");
   fprintf(stderr, "\t\t\ttlX static division with threads\n");
   fprintf(stderr, "\t\t\ttlpX static division with threads\n");
   fprintf(stderr, "\t\t\ttbX static division with threads and buffering\n");
   fprintf(stderr, "\t\t\ttdX initial static division but allow work stealing\n");
   fprintf(stderr, "\t\t\ttxX initial static division but allow direct work stealing\n");
   fprintf(stderr, "\t\t\tsinX no division of work using local queues\n");
   fprintf(stderr, "\t\t\tprX programmable scheduler\n");
   fprintf(stderr, "\t\t\tmpistaticX static division of work using mpi plus threads\n");
   fprintf(stderr, "\t\t\tmpidynamicX static division of work using mpi plus threads and work stealing\n");
   fprintf(stderr, "\t\t\tmpisingleX no division of work with static processes\n");
   fprintf(stderr, "\t-t \t\ttime execution\n");
   fprintf(stderr, "\t-m \t\tmemory statistics\n");
   fprintf(stderr, "\t-i <file>\tdump time statistics\n");
   fprintf(stderr, "\t-s \t\tshows database\n");
   fprintf(stderr, "\t-d \t\tdump database (debug option)\n");
   fprintf(stderr, "\t-h \t\tshow this screen\n");

   exit(EXIT_SUCCESS);
}

static inline bool
match_mpi(const char *name, char *arg, const scheduler_type type)
{
   const size_t len(strlen(name));
   
   if(strlen(arg) > len && strncmp(name, arg, len) == 0) {
      sched_type = type;
      arg += len;
      num_threads = (size_t)atoi(arg);
      return true;
   }
   
   return false;
}

static inline bool
match_threads(const char *name, char *arg, const scheduler_type type)
{
   return match_mpi(name, arg, type);
}

static inline bool
match_serial(const char *name, char *arg, const scheduler_type type)
{
   const size_t len(strlen(name));
   
   if(strlen(arg) == len && strncmp(name, arg, len) == 0) {
      sched_type = type;
      num_threads = 1;
      return true;
   }
   
   return false;
}

static inline bool
fail_sched(char* sched)
{
   fprintf(stderr, "Error: invalid scheduler %s\n", sched);
   exit(EXIT_FAILURE);
   return false;
}

static void
parse_sched(char *sched)
{
   assert(sched != NULL);
   
   if(strlen(sched) < 2)
      fail_sched(sched);
   
   // attempt to parse the scheduler string
   match_mpi("mpistatic", sched, SCHED_MPI_AND_THREADS_STATIC_LOCAL) ||
      match_mpi("mpidynamic", sched, SCHED_MPI_AND_THREADS_DYNAMIC_LOCAL) ||
      match_mpi("mpisingle", sched, SCHED_MPI_AND_THREADS_SINGLE_LOCAL) ||
      match_threads("tlp", sched, SCHED_THREADS_STATIC_LOCAL_PRIO) ||
      match_threads("tl", sched, SCHED_THREADS_STATIC_LOCAL) ||
      match_threads("tb", sched, SCHED_THREADS_STATIC_BUFF) ||
      match_threads("td", sched, SCHED_THREADS_DYNAMIC_LOCAL) ||
      match_threads("tx", sched, SCHED_THREADS_DIRECT_LOCAL) ||
      match_threads("sin", sched, SCHED_THREADS_SINGLE_LOCAL) ||
      match_threads("pr", sched, SCHED_THREADS_PROGRAMMABLE_LOCAL) ||
      match_serial("sl", sched, SCHED_SERIAL_LOCAL) ||
      fail_sched(sched);
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
               
            stat::set_stat_file(string(argv[1]));
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
      fprintf(stderr, "Error: please provide scheduler type and a program to run\n");
      exit(EXIT_FAILURE);
   } else if(program == NULL && sched_type != SCHED_UNKNOWN) {
      fprintf(stderr, "Error: please provide a program to run\n");
      exit(EXIT_FAILURE);
   } else if(program != NULL && sched_type == SCHED_UNKNOWN) {
      fprintf(stderr, "Error: please pick a scheduler to use\n");
      exit(EXIT_FAILURE);
   }
   
   if (num_threads == 0) {
      fprintf(stderr, "Error: invalid number of threads\n");
      exit(EXIT_FAILURE);
   }

   try {
      double start_time;
      execution_time tm;
      
      if(time_execution) {   
#ifdef COMPILE_MPI
         if(is_mpi_sched(sched_type))
            start_time = MPI_Wtime();
         else
#endif
         {
            tm.start();
         }
      }
			
      router rout(num_threads, argc, argv, is_mpi_sched(sched_type));

      machine mac(program, rout, num_threads, sched_type);

      if(show_database)
         mac.show_database();
      if(dump_database)
         mac.dump_database();
      if(memory_statistics)
         mac.show_memory();
      
      mac.start();

      if(time_execution) {
         if(is_mpi_sched(sched_type)) {
            double total_time(MPI_Wtime() - start_time);
            size_t ms = static_cast<size_t>(total_time * 1000);
            
            if(remote::self->get_rank() == 0)
               cout << "Time: " << ms << " ms" << endl;
         }
         else {
            tm.stop();
            size_t ms = tm.milliseconds();
            
            cout << "Time: " << ms << " ms" << endl;
         }
      }

   } catch(db::database_error& err) {
      cerr << "Database error: " << err.what() << endl;
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}
