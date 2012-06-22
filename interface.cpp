
#include <iostream>

#include <cstdlib>
#include <cstring>
#include <assert.h>

#include "interface.hpp"
#include "process/router.hpp"
#include "stat/stat.hpp"
#include "utils/time.hpp"
#include "utils/fs.hpp"
#include "process/machine.hpp"
#include "ui/manager.hpp"

using namespace process;
using namespace sched;
using namespace std;
using namespace utils;

scheduler_type sched_type = SCHED_UNKNOWN;
size_t num_threads = 0;
bool show_database = false;
bool dump_database = false;
bool time_execution = false;
bool memory_statistics = false;
bool running = false;
char *program_running = NULL;

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
	cerr << "Error: invalid scheduler " << sched << endl;
   exit(EXIT_FAILURE);
   return false;
}

void
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
      match_threads("td", sched, SCHED_THREADS_DYNAMIC_LOCAL) ||
      match_threads("tx", sched, SCHED_THREADS_DIRECT_LOCAL) ||
      match_threads("sin", sched, SCHED_THREADS_SINGLE_LOCAL) ||
      match_serial("sl", sched, SCHED_SERIAL_LOCAL) ||
		match_serial("ui", sched, SCHED_SERIAL_UI_LOCAL) ||
      fail_sched(sched);

	if (num_threads == 0) {
      cerr << "Error: invalid number of threads" << endl;
      exit(EXIT_FAILURE);
   }
}

void
help_schedulers(void)
{
	cerr << "\t-c <scheduler>\tselect scheduling type" << endl;
	cerr << "\t\t\tsl simple serial scheduler" << endl;
	cerr << "\t\t\tui serial scheduler + ui" << endl;
   cerr << "\t\t\ttlX static division with threads" << endl;
   cerr << "\t\t\ttlpX static division with threads" << endl;
	cerr << "\t\t\ttbX static division with threads and buffering" << endl;
	cerr << "\t\t\ttdX initial static division but allow work stealing" << endl;
	cerr << "\t\t\ttxX initial static division but allow direct work stealing" << endl;
	cerr << "\t\t\tsinX no division of work using local queues" << endl;
	cerr << "\t\t\tmpistaticX static division of work using mpi plus threads" << endl;
	cerr << "\t\t\tmpidynamicX static division of work using mpi plus threads and work stealing" << endl;
	cerr << "\t\t\tmpisingleX no division of work with static processes" << endl;
}

bool
run_program(int argc, char **argv, const char *program, const vm::machine_arguments& margs)
{
	assert(utils::file_exists(string(program)));

	try {
      double start_time;
      execution_time tm;
      
      if(time_execution) {
         if(is_mpi_sched(sched_type))
            start_time = MPI_Wtime();
         else
         {
            tm.start();
         }
      }

		running = true;
		program_running = (char*)program;
		LOG_PROGRAM_RUNNING();
		
      router rout(num_threads, argc, argv, is_mpi_sched(sched_type));
      machine mac(program, rout, num_threads, sched_type, margs);
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
      exit(EXIT_FAILURE);
   }

	running = false;
	program_running = NULL;
	LOG_PROGRAM_STOPPED();

	return true;
}
