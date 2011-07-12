#include <iostream>
#include <signal.h>

#include "process/machine.hpp"
#include "vm/program.hpp"
#include "vm/state.hpp"
#include "process/process.hpp"
#include "runtime/list.hpp"
#include "sched/mpi/message.hpp"
#include "mem/thread.hpp"
#include "mem/stat.hpp"
#include "stat/stat.hpp"

using namespace process;
using namespace db;
using namespace std;
using namespace vm;
using namespace boost;
using namespace sched;
using namespace mem;
using namespace utils;
using namespace stat;

namespace process
{
   
bool
machine::same_place(const node::node_id id1, const node::node_id id2) const
{
   if(id1 == id2)
      return true;
   
   remote *rem1(rout.find_remote(id1));
   remote *rem2(rout.find_remote(id2));
   
   if(rem1 != rem2)
      return false;
   
   return rem1->find_proc_owner(id1) == rem1->find_proc_owner(id2);
}

void
machine::route_self(process *proc, node *node, const simple_tuple *stpl)
{
   proc->get_scheduler()->new_work_self(node, stpl, false);
}

void
machine::route(process *caller, const node::node_id id, const simple_tuple* stuple)
{  
   remote* rem(rout.find_remote(id));
   sched::base *sched_caller(caller->get_scheduler());
   
   assert(id <= state::DATABASE->max_id());
   
   if(rem == remote::self) {
      // on this machine
      
      node *node(state::DATABASE->find_node(id));
      
      sched::base *sched_other(sched_caller->find_scheduler(id));
      
      node->more_to_process(stuple->get_predicate_id());
      
      if(sched_caller == sched_other)
         sched_caller->new_work(NULL, node, stuple);
      else
         sched_caller->new_work_other(sched_other, node, stuple);
   }
#ifdef COMPILE_MPI
   else {
      // remote, mpi machine
      
      assert(rout.use_mpi());
      
      message *msg(new message(id, stuple));
      
      sched_caller->new_work_remote(rem, id, msg);
   }
#endif
}

static inline string
get_output_filename(const string other, const remote::remote_id id)
{
   return string("meld_output." + other + "." + to_string(id));
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

void
machine::slice_function(void)
{
   bool tofinish(false);
   
   // enable SIGALRM and SIGUSR1
   sigset_t set;
   sigemptyset(&set);
   sigaddset(&set, SIGALRM);
   sigaddset(&set, SIGUSR1);
   pthread_sigmask(SIG_UNBLOCK, &set, NULL);
   
   int sig;
   
   set_timer();
   
   while (true) {
      
      sigwait(&set, &sig);
      
      switch(sig) {
         case SIGALRM:
         if(tofinish)
            return;
         slices.beat();
         set_timer();
         break;
         case SIGUSR1:
         tofinish = true;
         break;
         default: assert(false);
      }
   }
}

void
machine::start(void)
{
   deactivate_signals();
   
   if(stat_enabled()) {
      // initiate alarm thread
      alarm_thread = new boost::thread(bind(&machine::slice_function, this));
   }
   
   for(size_t i(1); i < num_threads; ++i)
      process_list[i]->start();
   process_list[0]->start();
   
   for(size_t i(1); i < num_threads; ++i)
      process_list[i]->join();
   
   if(alarm_thread) {
      kill(getpid(), SIGUSR1);
      alarm_thread->join();
      delete alarm_thread;
      alarm_thread = NULL;
      slices.write(get_stat_file(), sched_type);
   }

   const bool will_print(will_show_database || will_dump_database);

   if(will_print) {
      if(rout.use_mpi()) {
         if(will_show_database) {
            const string filename(get_output_filename("db", remote::self->get_rank()));
            ofstream fp(filename.c_str());
            
            state::DATABASE->print_db(fp);
         }
         if(will_dump_database) {
            const string filename(get_output_filename("dump", remote::self->get_rank()));
            ofstream fp(filename.c_str());
            state::DATABASE->dump_db(fp);
         }
         rout.barrier();
         
         // read and output files
         if(remote::self->is_leader()) {
            if(will_show_database) {
               for(size_t i(0); i < remote::world_size; ++i)
                  file_print_and_remove(get_output_filename("db", i));
            }
            if(will_dump_database) {
               for(size_t i(0); i < remote::world_size; ++i)
                  file_print_and_remove(get_output_filename("dump", i));
            }
         }
      } else {
         if(will_show_database)
            state::DATABASE->print_db(cout);
         if(will_dump_database)
            state::DATABASE->dump_db(cout);
      }
   }

   if(will_show_memory) {
#ifdef MEMORY_STATISTICS
      cout << "Total memory in use: " << get_memory_in_use() / 1024 << "KB" << endl;
#else
      cout << "Memory statistics support was not compiled in" << endl;
#endif
   }
}

static inline database::create_node_fn
get_creation_function(const scheduler_type sched_type)
{
   switch(sched_type) {
      case SCHED_THREADS_STATIC_GLOBAL:
      case SCHED_MPI_AND_THREADS_STATIC_GLOBAL:
         return database::create_node_fn(sched::static_global::create_node);
      case SCHED_THREADS_STATIC_LOCAL:
      case SCHED_THREADS_SINGLE_LOCAL:
         return database::create_node_fn(sched::static_local::create_node);
      case SCHED_THREADS_DYNAMIC_LOCAL:
         return database::create_node_fn(sched::dynamic_local::create_node);
      case SCHED_MPI_AND_THREADS_STATIC_LOCAL:
         return database::create_node_fn(sched::mpi_thread_static::create_node);
      case SCHED_MPI_AND_THREADS_DYNAMIC_LOCAL:
         return database::create_node_fn(sched::mpi_thread_dynamic::create_node);
      case SCHED_MPI_AND_THREADS_SINGLE_LOCAL:
         return database::create_node_fn(sched::mpi_thread_single::create_node);
      case SCHED_UNKNOWN:
         return NULL;
   }
   
   throw machine_error("unknown scheduler type");
}

machine::machine(const string& file, router& _rout, const size_t th, const scheduler_type _sched_type):
   filename(file),
   num_threads(th),
   sched_type(_sched_type),
   will_show_database(false),
   will_dump_database(false),
   will_show_memory(false),
   rout(_rout),
   alarm_thread(NULL),
   slices(th)
{  
   new program(filename);
   
   state::DATABASE = new database(filename, get_creation_function(_sched_type));
   state::NUM_THREADS = num_threads;
   state::MACHINE = this;
   
   mem::init(num_threads);
   process_list.resize(num_threads);
   
   vector<sched::base*> schedulers;
   
   switch(sched_type) {
      case SCHED_MPI_AND_THREADS_STATIC_GLOBAL:
         schedulers = sched::mpi_static::start(num_threads);
         break;
      case SCHED_THREADS_STATIC_LOCAL:
         schedulers = sched::static_local::start(num_threads);
         break;
      case SCHED_THREADS_STATIC_GLOBAL:
         schedulers = sched::static_global::start(num_threads);
         break;
      case SCHED_THREADS_SINGLE_LOCAL:
         schedulers = sched::threads_single::start(num_threads);
         break;
      case SCHED_THREADS_DYNAMIC_LOCAL:
         schedulers = sched::dynamic_local::start(num_threads);
         break;
      case SCHED_MPI_AND_THREADS_STATIC_LOCAL:
         schedulers = sched::mpi_thread_static::start(num_threads);
         break;
      case SCHED_MPI_AND_THREADS_DYNAMIC_LOCAL:
         schedulers = sched::mpi_thread_dynamic::start(num_threads);
         break;
      case SCHED_MPI_AND_THREADS_SINGLE_LOCAL:
         schedulers = sched::mpi_thread_single::start(num_threads);
         break;
      case SCHED_UNKNOWN: assert(false); break;
   }
   
   assert(schedulers.size() == num_threads);
   
   for(process_id i(0); i < num_threads; ++i)
      process_list[i] = new process(i, schedulers[i]);
}

machine::~machine(void)
{
   delete state::PROGRAM;
   delete state::DATABASE;
   
   for(process_id i(0); i != num_threads; ++i)
      delete process_list[i];
      
   if(alarm_thread)
      delete alarm_thread;
      
   mem::cleanup(num_threads);
}

}
