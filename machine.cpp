#include <iostream>
#include <signal.h>
#include <unistd.h>

#include "machine.hpp"
#include "vm/program.hpp"
#include "vm/state.hpp"
#include "vm/exec.hpp"
#include "mem/thread.hpp"
#include "mem/stat.hpp"
#include "stat/stat.hpp"
#include "utils/fs.hpp"
#include "interface.hpp"
#include "thread/threads.hpp"
#include "runtime/objs.hpp"

using namespace process;
using namespace db;
using namespace std;
using namespace vm;
using namespace sched;
using namespace mem;
using namespace utils;
using namespace statistics;

namespace process
{
   
void
machine::run_action(sched::threads_sched *sched, vm::tuple *tpl, vm::predicate *pred,
      candidate_gc_nodes& gc_nodes)
{
   (void)sched;

	const predicate_id pid(pred->get_id());
	
	assert(pred->is_action_pred());
	
   switch(pid) {
      case SETCOLOR_PREDICATE_ID:
      break;
      case SETCOLOR2_PREDICATE_ID:
      break;
      case SETEDGELABEL_PREDICATE_ID:
      break;
      case WRITE_STRING_PREDICATE_ID: {
         runtime::rstring::ptr s(tpl->get_string(0));

         cout << s->get_content() << endl;
        }
      break;
      default:
         cerr << "Cannot execute action " << pred->get_name() << endl;
         assert(false);
         break;
   }

   vm::tuple::destroy(tpl, pred, gc_nodes);
}

void
machine::deactivate_signals(void)
{
   sigset_t set;
   
   sigemptyset(&set);
   sigaddset(&set, SIGALRM);
   sigaddset(&set, SIGUSR1);
   
   sigprocmask(SIG_BLOCK, &set, NULL);
}

void
machine::set_timer(void)
{
   // pre-compute the number of usecs from msecs
   static long usec = SLICE_PERIOD * 1000;
   struct itimerval t;
   
   t.it_interval.tv_sec = 0;
   t.it_interval.tv_usec = 0;
   t.it_value.tv_sec = 0;
   t.it_value.tv_usec = usec;
   
   setitimer(ITIMER_REAL, &t, 0);
}

#ifdef INSTRUMENTATION
void
machine::slice_function(void)
{
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
      
      switch(sig) {
         case SIGALRM:
            if(tofinish)
               return;
            slices.beat(all);
            set_timer();
            break;
         case SIGUSR1:
            tofinish = true;
            break;
         default: assert(false);
      }
   }
}
#endif

void
machine::execute_const_code(void)
{
	state st;
	
	// no node or tuple whatsoever
	st.setup(nullptr, POSITIVE_DERIVATION, 0);
	
	execute_process(all->PROGRAM->get_const_bytecode(), st, nullptr, nullptr);
}

void
machine::init_sched(const process_id id)
{
   // ensure own memory pool
   mem::ensure_pool();
#ifdef INSTRUMENTATION
   All->THREAD_POOLS[id] = mem::mem_pool;
#endif
#ifdef LOCK_STATISTICS
   utils::all_stats[id] = new utils::lock_stat();
   utils::_stat = utils::all_stats[id];
#endif

   all->SCHEDS[id] = new sched::threads_sched(id);
   all->SCHEDS[id]->loop();
}

void
machine::start(void)
{
	// execute constants code
	execute_const_code();
	
   deactivate_signals();
#ifdef INSTRUMENTATION
   all->THREAD_POOLS.resize(all->NUM_THREADS, NULL);
   
   if(stat_enabled())
      // initiate alarm thread
      alarm_thread = new thread(bind(&machine::slice_function, this));
#endif

   sched::threads_sched::init_barriers(all->NUM_THREADS);
#ifdef LOCK_STATISTICS
   utils::all_stats.resize(all->NUM_THREADS, NULL);
#endif
   
   thread *threads[all->NUM_THREADS];
   for(process_id i(1); i < all->NUM_THREADS; ++i) {
      threads[i] = new thread(&machine::init_sched, this, i);
   }
   init_sched(0);
   
   for(size_t i(1); i < all->NUM_THREADS; ++i)
      threads[i]->join();

   // join dynamic node information
   for(size_t i(1); i < all->NUM_THREADS; ++i)
      all->SCHEDS[0]->merge_new_nodes(*(all->SCHEDS[i]));
   all->SCHEDS[0]->commit_nodes();

#if 0
   cout << "Total Facts: " << this->all->DATABASE->total_facts() << endl;
   cout << "Total Nodes: " << this->all->DATABASE->num_nodes() << endl;
   cout << "Bytes used: " << bytes_used << endl;
#endif
#ifdef LOCK_STATISTICS
   utils::lock_stat *mstat(utils::mutex::merge_stats());
   utils::mutex::print_statistics(mstat);
#endif
#ifdef FACT_STATISTICS
   uint64_t facts_derived(0);
   uint64_t facts_consumed(0);
   uint64_t facts_sent(0);
   uint64_t count_add_work_self(0);
   uint64_t count_add_work_other(0);
   uint64_t count_stolen_nodes(0);
   uint64_t count_set_priority(0);
   uint64_t count_add_priority(0);
   for(size_t i(0); i < all->NUM_THREADS; ++i) {
      facts_derived += all->SCHEDS[i]->get_state().facts_derived;
      facts_consumed += all->SCHEDS[i]->get_state().facts_consumed;
      facts_sent += all->SCHEDS[i]->get_state().facts_sent;
      count_add_work_self += all->SCHEDS[i]->count_add_work_self;
      count_add_work_other += all->SCHEDS[i]->count_add_work_other;
      count_stolen_nodes += all->SCHEDS[i]->count_stolen_nodes;
      count_set_priority += all->SCHEDS[i]->count_set_priority;
      count_add_priority += all->SCHEDS[i]->count_add_priority;
   }
   cerr << "derived, consumed, sent,addself, addother, stolen, setprio, addprio" << endl;
   cerr << facts_derived << ", " << facts_consumed << ", " << facts_sent <<
      ", " << count_add_work_self << ", " << count_add_work_other << ", " <<
      count_stolen_nodes << ", " << count_set_priority << ", " <<
      count_add_priority << endl;
#endif

   for(size_t i(1); i < all->NUM_THREADS; ++i)
      delete threads[i];
      
#ifdef INSTRUMENTATION
   if(alarm_thread) {
      kill(getpid(), SIGUSR1);
      alarm_thread->join();
      delete alarm_thread;
      alarm_thread = NULL;
      slices.write(get_stat_file(), all);
   }
#endif

   const bool will_print(show_database || dump_database);

   if(will_print) {
      if(show_database)
         all->DATABASE->print_db(cout);
      if(dump_database)
         all->DATABASE->dump_db(cout);
   }

#ifdef MEMORY_STATISTICS
   cout << "Total memory allocated: " << get_total_memory() / 1024 << "KB" << endl;
   cout << "Total memory in use: " << get_memory_in_use() / 1024 << "KB" << endl;
   cout << "Malloc()'s called: " << get_num_mallocs() << endl;
#else
#endif
}

machine::machine(const string& file, const size_t th,
		const machine_arguments& margs, const string& data_file):
   all(new vm::all()),
   filename(file)
#ifdef INSTRUMENTATION
   , alarm_thread(NULL)
   , slices(th)
#endif
{
   mem::ensure_pool();

   init_types();
   init_external_functions();

   bool added_data_file(false);

   All = all;
   this->all->PROGRAM = new vm::program(file);
   theProgram = this->all->PROGRAM;
   if(this->all->PROGRAM->is_data())
      throw machine_error(string("cannot run data files"));
   if(data_file != string("")) {
      if(file_exists(data_file)) {
         vm::program data(data_file);
         if(!this->all->PROGRAM->add_data_file(data)) {
            throw machine_error(string("could not import data file"));
         }
         added_data_file = true;
      } else {
         throw machine_error(string("data file ") + data_file + string(" not found"));
      }
   }

   if(margs.size() < this->all->PROGRAM->num_args_needed())
      throw machine_error(string("this program requires ") + utils::to_string(all->PROGRAM->num_args_needed()) + " arguments");

   this->all->set_arguments(margs);
   this->all->DATABASE = new database(added_data_file ? data_file : filename);
   this->all->NUM_THREADS = th;
   this->all->NUM_THREADS_NEXT_UINT = next_multiple_of_uint(th);
   this->all->MACHINE = this;
   this->all->SCHEDS.resize(th, NULL);

    nodes_per_thread = total_nodes() / num_threads;
    if(nodes_per_thread * num_threads < total_nodes())
       nodes_per_thread++;
}

machine::~machine(void)
{
   for(process_id i(0); i != all->NUM_THREADS; ++i)
      delete all->SCHEDS[i];

   // when deleting database, we need to access the program,
   // so we must delete this in correct order
   delete all->DATABASE;
   
   delete all->PROGRAM;
      
#ifdef INSTRUMENTATION
   if(alarm_thread)
      delete alarm_thread;
#endif
}

}
