
#include <iostream>

#include <cstdlib>
#include <cstring>
#include <assert.h>
#include <unistd.h>

#include "interface.hpp"
#include "stat/stat.hpp"
#include "utils/time.hpp"
#include "utils/fs.hpp"
#include "machine.hpp"
#include "vm/reader.hpp"

using namespace process;
using namespace sched;
using namespace std;
using namespace utils;
using namespace vm;

size_t num_threads = 0;
bool show_database = false;
bool dump_database = false;
bool time_execution = false;
bool scheduling_mechanism = true;
bool work_stealing = true;

static inline size_t num_cpus_available(void) {
   return (size_t)sysconf(_SC_NPROCESSORS_ONLN);
}

static inline bool match_mpi(const char* name, char* arg) {
   const size_t len(strlen(name));

   if (strlen(arg) == len && strncmp(name, arg, len) == 0) {
      arg += len;
      num_threads = num_cpus_available();
      return true;
   } else if (strlen(arg) > len && strncmp(name, arg, len) == 0) {
      arg += len;
      num_threads = (size_t)atoi(arg);
      return true;
   }

   return false;
}

static inline bool match_threads(const char* name, char* arg) {
   return match_mpi(name, arg);
}

static inline bool fail_sched(char* sched) {
   cerr << "Error: invalid scheduler " << sched << endl;
   exit(EXIT_FAILURE);
   return false;
}

void parse_sched(char* sched) {
   assert(sched != NULL);

   if (strlen(sched) < 2) fail_sched(sched);

   // attempt to parse the scheduler string
   match_threads("th", sched) || fail_sched(sched);

   if (num_threads == 0) {
      cerr << "Error: invalid number of threads" << endl;
      exit(EXIT_FAILURE);
   }
}

void help_schedulers(void) {
   cerr << "\t-c <scheduler>\tselect scheduling type" << endl;
   cerr << "\t\t\tthX multithreaded scheduler with task stealing" << endl;
}

static inline void finish(void) {}

bool run_program(machine& mac) {
   try {
      double start_time(0.0);
      execution_time tm;

      (void)start_time;

      if (time_execution) tm.start();

      mac.start();

      if (time_execution) {
         tm.stop();
         size_t ms = tm.milliseconds();

         cout << "Time: " << ms << " ms" << endl;
      }

   } catch (machine_error& err) {
      finish();
      throw err;
   } catch (load_file_error& err) {
      finish();
      throw err;
   } catch (db::database_error& err) {
      finish();
      throw err;
   }

   finish();

   return true;
}
