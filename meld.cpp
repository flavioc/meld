
#include <iostream>

#include "process/machine.hpp"
#include "runtime/list.hpp"
#include "utils/utils.hpp"
#include "process/router.hpp"
#include "db/trie.hpp"

using namespace utils;
using namespace process;
using namespace std;

static size_t num_threads = 0;
static char *program = NULL;
static char *progname = NULL;
static scheduler_type sched_type = SCHED_UNKNOWN;
static bool show_database = false;
static bool dump_database = false;
static bool time_execution = false;

boost::mutex allocator_mtx;
std::tr1::unordered_set<void*> mem_set;

static void
help(void)
{
   fprintf(stderr, "meld: execute meld program\n");
   fprintf(stderr, "\t-f <name>\tmeld program\n");
   fprintf(stderr, "\t-c <scheduler>\tselect scheduling type\n");
   fprintf(stderr, "\t\t\ttsX static division of work with X threads\n");
   fprintf(stderr, "\t\t\tstealX initial static division but allow work stealing\n");
   fprintf(stderr, "\t\t\tmpi static division of work using mpi (use mpirun)\n");
   fprintf(stderr, "\t-t \t\ttime execution\n");
   fprintf(stderr, "\t-s \t\tshows database\n");
   fprintf(stderr, "\t-d \t\tdump database (debug option)\n");
   fprintf(stderr, "\t-h \t\tshow this screen\n");

   exit(EXIT_SUCCESS);
}

static void
parse_sched(char *sched)
{
   assert(sched != NULL);
   
   if(strlen(sched) < 3) {
      fprintf(stderr, "Error: invalid scheduler %s\n", sched);
      exit(EXIT_FAILURE);
   }
      
   if(strncmp(sched, "ts", 2) == 0) {
      sched += 2;
      num_threads = (size_t)atoi(sched);
      sched_type = SCHED_THREADS_STATIC;
   } else if(strlen(sched) == 3 && strncmp(sched, "mpi", 3) == 0) {
#ifndef COMPILE_MPI
      fprintf(stderr, "Error: MPI support was not compiled\n");
      exit(EXIT_FAILURE);
#endif
      sched_type = SCHED_MPI_UNI_STATIC;
      num_threads = 1;
   } else if(strlen(sched) > 5 && strncmp(sched, "steal", 5) == 0) {
      sched_type = SCHED_THREADS_STEALER;
      sched += 5;
      num_threads = (size_t)atoi(sched);
   } else {
      fprintf(stderr, "Error: invalid scheduler %s\n", sched);
      exit(EXIT_FAILURE);
   }
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
      
   if(sched_type == SCHED_MPI_UNI_STATIC)
	   num_threads = 1;

   try {
      double start_time;
      execution_time tm;
      
      if(time_execution) {   
#ifdef COMPILE_MPI
         if(sched_type == SCHED_MPI_UNI_STATIC)
            start_time = MPI_Wtime();
         else
#endif
         {
            tm.start();
         }
      }
			
      router rout(num_threads, argc, argv, sched_type == SCHED_MPI_UNI_STATIC);

      machine mac(program, rout, num_threads, sched_type);

      if(show_database)
         mac.show_database();
      if(dump_database)
         mac.dump_database();
      
      mac.start();

      if(time_execution) {
         size_t ms;
         
#ifdef COMPILE_MPI
         if(sched_type == SCHED_MPI_UNI_STATIC) {
            double total_time(MPI_Wtime() - start_time);
            ms = static_cast<size_t>(total_time * 1000);
            
            if(remote::self->get_rank() == 0)
               cout << "Time: " << ms << " ms" << endl;
         }
         else
#endif
         {
            tm.stop();
            ms = tm.milliseconds();
            
            cout << "Time: " << ms << " ms" << endl;
         }
      }

   } catch(db::database_error& err) {
      cerr << "Database error: " << err.what() << endl;
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}
