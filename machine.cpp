#include <iostream>
#include <signal.h>
#include <unistd.h>

#include "incbin.h"
#include "machine.hpp"
#include "vm/program.hpp"
#include "vm/state.hpp"
#include "vm/exec.hpp"
#include "mem/thread.hpp"
#include "mem/stat.hpp"
#include "stat/stat.hpp"
#include "utils/fs.hpp"
#include "utils/random.hpp"
#include "interface.hpp"
#include "runtime/objs.hpp"

using namespace process;
using namespace db;
using namespace std;
using namespace vm;
using namespace sched;
using namespace mem;
using namespace utils;
using namespace statistics;

namespace process {

void machine::run_action(sched::thread* sched, vm::tuple* tpl,
                         vm::predicate* pred, mem::node_allocator *alloc,
                         candidate_gc_nodes& gc_nodes) {
   (void)sched;

   assert(pred->is_action_pred());

   if (pred->get_name() == "setcolor")
      ;
   else if (pred->get_name() == "setcolor2")
      ;
   else if (pred->get_name() == "setedgelabel")
      ;
   else if (pred->get_name() == "write-string") {
      runtime::rstring::ptr s(tpl->get_string(0));
      cout << s->get_content() << endl;
   } else {
      cerr << "Cannot execute action " << pred->get_name() << endl;
      assert(false);
   }

   vm::tuple::destroy(tpl, pred, alloc, gc_nodes);
}

void machine::deactivate_signals(void) {
   sigset_t set;

   sigemptyset(&set);
   sigaddset(&set, SIGALRM);
   sigaddset(&set, SIGUSR1);

   sigprocmask(SIG_BLOCK, &set, nullptr);
}

void machine::set_timer(void) {
   // pre-compute the number of usecs from msecs
   static long usec = SLICE_PERIOD * 1000;
   struct itimerval t;

   t.it_interval.tv_sec = 0;
   t.it_interval.tv_usec = 0;
   t.it_value.tv_sec = 0;
   t.it_value.tv_usec = usec;

   setitimer(ITIMER_REAL, &t, nullptr);
}

#ifdef INSTRUMENTATION
void machine::slice_function(void) {
   bool tofinish(false);

   // add SIGALRM and SIGUSR1 to sigset
   // to be used by sigwait
   sigset_t set;
   sigemptyset(&set);
   sigaddset(&set, SIGALRM);
   sigaddset(&set, SIGUSR1);

   int sig;

   set_timer();

   while (true) {
      const int ret(sigwait(&set, &sig));

      (void)ret;
      assert(ret == 0);

      switch (sig) {
         case SIGALRM:
            if (tofinish) return;
            slices.beat(all);
            set_timer();
            break;
         case SIGUSR1:
            tofinish = true;
            break;
         default:
            assert(false);
      }
   }
}
#endif

void machine::init_sched(const process_id id) {
   // ensure own memory pool
   mem::ensure_pool();
#ifdef INSTRUMENTATION
   All->THREAD_POOLS[id] = mem::mem_pool;
#endif
#if defined(LOCK_STATISTICS) || defined(FACT_STATISTICS)
   utils::all_stats[id] = new utils::lock_stat();
   utils::_stat = utils::all_stats[id];
#endif
   all->SCHEDS[id] = new sched::thread(id);
   utils::set_random_generator(all->SCHEDS[id]->get_random());
   all->SCHEDS[id]->loop();
   all->SCHEDS[id]->commit_nodes();
#ifdef MEMORY_STATISTICS
   merge_memory_statistics();
#endif
}

void machine::start(void) {
   deactivate_signals();
#ifdef INSTRUMENTATION
   all->THREAD_POOLS.resize(all->NUM_THREADS, NULL);

   if (stat_enabled())
      // initiate alarm thread
      alarm_thread = new std::thread(bind(&machine::slice_function, this));
#endif

   sched::thread::init_barriers(all->NUM_THREADS);
#if defined(LOCK_STATISTICS) || defined(FACT_STATISTICS)
   utils::all_stats.resize(all->NUM_THREADS, NULL);
#endif

   std::thread* threads[all->NUM_THREADS];
   for (process_id i(1); i < all->NUM_THREADS; ++i) {
      threads[i] = new std::thread(&machine::init_sched, this, i);
   }
   init_sched(0);

   for (size_t i(1); i < all->NUM_THREADS; ++i) threads[i]->join();

#if 0
   cout << "Total Facts: " << this->all->DATABASE->total_facts() << endl;
   cout << "Total Nodes: " << this->all->DATABASE->num_nodes() << endl;
   cout << "Bytes used: " << bytes_used << endl;
#endif
#if defined(LOCK_STATISTICS) || defined(FACT_STATISTICS)
   utils::lock_stat* mstat(utils::mutex::merge_stats());
   utils::mutex::print_statistics(mstat);
#ifdef FACT_STATISTICS
   cout << "facts_end: " << vm::All->DATABASE->total_facts() << endl;
#endif
#endif

   for (size_t i(1); i < all->NUM_THREADS; ++i) delete threads[i];

#ifdef INSTRUMENTATION
   if (alarm_thread) {
      kill(getpid(), SIGUSR1);
      alarm_thread->join();
      delete alarm_thread;
      alarm_thread = NULL;
      slices.write(get_stat_file(), all);
   }
#endif

   const bool will_print(show_database || dump_database);

   if (will_print) {
      if (show_database) all->DATABASE->print_db(cout);
      if (dump_database) all->DATABASE->dump_db(cout);
   }

#ifdef MEMORY_STATISTICS
   print_memory_statistics();
#endif
}

void machine::setup_threads(const size_t th) {
   this->all->NUM_THREADS = th;
   this->all->NUM_THREADS_NEXT_UINT = next_multiple_of_uint(th);
   this->all->MACHINE = this;
   this->all->SCHEDS.resize(th, nullptr);

   nodes_per_thread = total_nodes() / num_threads;
   if(nodes_per_thread * num_threads < total_nodes())
      nodes_per_thread++;
}

void machine::init(const machine_arguments& margs) {
   mem::ensure_pool();
   All = all = new vm::all();

   init_types();
   init_external_functions();
   all->set_arguments(margs);
}

#ifdef COMPILED
INCBIN_EXTERN(Axioms);

machine::machine(const size_t th, const machine_arguments& margs)
#ifdef INSTRUMENTATION
    : slices(th)
#endif
{
#ifdef COMPILED
   init(margs);
   theProgram = all->PROGRAM = new vm::program();
   auto iss(compiled_database_stream());
   all->DATABASE = new database(*iss);
   setup_threads(th);
#else
   (void)th;
   (void)margs;
#endif
}

#else
machine::machine(const string& file, const size_t th,
                 const machine_arguments& margs, const string& data_file)
    : filename(file)
#ifdef INSTRUMENTATION
      ,
      slices(th)
#endif
{
   init(margs);
   bool added_data_file(false);

   theProgram = all->PROGRAM = new vm::program(file);

   if (this->all->PROGRAM->is_data())
      throw machine_error(string("cannot run data files"));
   if (data_file != string("")) {
      if (file_exists(data_file)) {
         vm::program data(data_file);
         if (!this->all->PROGRAM->add_data_file(data)) {
            throw machine_error(string("could not import data file"));
         }
         added_data_file = true;
      } else {
         throw machine_error(string("data file ") + data_file +
                             string(" not found"));
      }
   }

   all->check_arguments(theProgram->num_args_needed());
   auto fp(
       program::bypass_bytecode_header(added_data_file ? data_file : filename));
   all->DATABASE = new database(*fp);
   setup_threads(th);
}
#endif

machine::~machine(void) {
   for (process_id i(0); i != all->NUM_THREADS; ++i) delete all->SCHEDS[i];

   // when deleting database, we need to access the program,
   // so we must delete this in correct order
   delete all->DATABASE;

   delete all->PROGRAM;

//   mem::dump_pool();

#ifdef INSTRUMENTATION
   if (alarm_thread) delete alarm_thread;
#endif
}
}
