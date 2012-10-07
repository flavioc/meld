#include <iostream>
#include <signal.h>

#include "process/machine.hpp"
#include "process/process.hpp"
#include "vm/program.hpp"
#include "vm/state.hpp"
#include "vm/exec.hpp"
#include "runtime/list.hpp"
#include "sched/mpi/message.hpp"
#include "mem/thread.hpp"
#include "mem/stat.hpp"
#include "stat/stat.hpp"
#include "utils/fs.hpp"
#include "interface.hpp"
#include "sched/serial.hpp"
#include "sched/serial_ui.hpp"
#include "thread/mpi_dynamic.hpp"
#include "thread/mpi_single.hpp"
#include "thread/mpi_static.hpp"
#include "thread/static_prio.hpp"
#include "thread/direct.hpp"
#include "thread/dynamic.hpp"
#include "thread/static.hpp"
#include "thread/single.hpp"

using namespace process;
using namespace db;
using namespace std;
using namespace vm;
using namespace boost;
using namespace sched;
using namespace mem;
using namespace utils;
using namespace statistics;

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
machine::route_self(process *proc, node *node, simple_tuple *stpl)
{
	sched::base *sched(proc->get_scheduler());
	const predicate_id pid(stpl->get_predicate_id());

	if(pid == SET_PRIORITY_PREDICATE_ID) {
		vm::tuple *tpl(stpl->get_tuple());
		sched->set_node_priority(node, tpl->get_int(0));
		delete tpl;
		delete stpl;
	} else if(pid == ADD_PRIORITY_PREDICATE_ID) {
		vm::tuple *tpl(stpl->get_tuple());
		sched->add_node_priority(node, tpl->get_int(0));
		delete tpl;
		delete stpl;
	} else if(pid == WRITE_STRING_PREDICATE_ID) {
		vm::tuple *tpl(stpl->get_tuple());
		runtime::rstring::ptr s(tpl->get_string(0));

		cout << s->get_content() << endl;

		delete tpl;
		delete stpl;
	} else {
		sched->new_work_self(node, stpl);
	}
}

void
machine::route(const node* from, process *caller, const node::node_id id, simple_tuple* stpl)
{  
   remote* rem(rout.find_remote(id));
   sched::base *sched_caller(caller->get_scheduler());
   
   assert(sched_caller != NULL);
   assert(id <= state::DATABASE->max_id());
   
   if(rem == remote::self) {
      // on this machine
      
      node *node(state::DATABASE->find_node(id));
      
      sched::base *sched_other(sched_caller->find_scheduler(node));

		if(stpl->get_predicate_id() == SET_PRIORITY_PREDICATE_ID) {
			vm::tuple *tpl(stpl->get_tuple());
			sched_other->set_node_priority(node, tpl->get_int(0));
			delete tpl;
			delete stpl;
		} else if(stpl->get_predicate_id() == ADD_PRIORITY_PREDICATE_ID) {
			vm::tuple *tpl(stpl->get_tuple());
			sched_other->add_node_priority(node, tpl->get_int(0));
			delete tpl;
			delete stpl;
		} else {
			work new_work(node, stpl);
      
			if(sched_caller == sched_other)
				sched_caller->new_work(from, new_work);
			else {
				sched_comm(sched_caller);
				sched_caller->new_work_other(sched_other, new_work);
				sched_active(sched_caller);
			}
		}
   }
#ifdef COMPILE_MPI
   else {
      // remote, mpi machine
      
      assert(rout.use_mpi());
      
      message *msg(new message(id, stpl));
      
      sched_comm(sched_caller);
      sched_caller->new_work_remote(rem, id, msg);
      sched_active(sched_caller);
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
		
		assert(ret == 0);
      
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
machine::execute_const_code(void)
{
	state st;
	
	// no node or tuple whatsoever
	st.setup(NULL, NULL, 0);
	
	execute_bytecode(state::PROGRAM->get_const_bytecode(), st);
}

void
machine::start(void)
{
	// execute constants code
	execute_const_code();
	
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
      
#ifndef NDEBUG
   for(size_t i(1); i < num_threads; ++i)
      assert(process_list[i-1]->num_iterations() == process_list[i]->num_iterations());
   if(state::PROGRAM->is_safe())
      assert(process_list[0]->num_iterations() == 1);
#endif
   
   if(alarm_thread) {
      kill(getpid(), SIGUSR1);
      alarm_thread->join();
      delete alarm_thread;
      alarm_thread = NULL;
      slices.write(get_stat_file(), sched_type);
   }

   const bool will_print(show_database || dump_database);

   if(will_print) {
      if(rout.use_mpi()) {
         if(show_database) {
            const string filename(get_output_filename("db", remote::self->get_rank()));
            ofstream fp(filename.c_str());
            
            state::DATABASE->print_db(fp);
         }
         if(dump_database) {
            const string filename(get_output_filename("dump", remote::self->get_rank()));
            ofstream fp(filename.c_str());
            state::DATABASE->dump_db(fp);
         }
         
         rout.barrier();
         
         // read and output files
         if(remote::self->is_leader()) {
            if(show_database) {
               for(size_t i(0); i < remote::world_size; ++i)
                  file_print_and_remove(get_output_filename("db", i));
            }
            if(dump_database) {
               for(size_t i(0); i < remote::world_size; ++i)
                  file_print_and_remove(get_output_filename("dump", i));
            }
         }
      } else {
         if(show_database)
            state::DATABASE->print_db(cout);
         if(dump_database)
            state::DATABASE->dump_db(cout);
      }
   }

   if(memory_statistics) {
#ifdef MEMORY_STATISTICS
      cout << "Total memory in use: " << get_memory_in_use() / 1024 << "KB" << endl;
      cout << "Malloc()'s called: " << get_num_mallocs() << endl;
#else
      cout << "Memory statistics support was not compiled in" << endl;
#endif
   }
}

static inline database::create_node_fn
get_creation_function(const scheduler_type sched_type)
{
   switch(sched_type) {
      case SCHED_THREADS_STATIC_LOCAL:
      case SCHED_THREADS_SINGLE_LOCAL:
         return database::create_node_fn(sched::static_local::create_node);
      case SCHED_THREADS_STATIC_LOCAL_PRIO:
         return database::create_node_fn(sched::static_local_prio::create_node);
      case SCHED_THREADS_DYNAMIC_LOCAL:
         return database::create_node_fn(sched::dynamic_local::create_node);
      case SCHED_THREADS_DIRECT_LOCAL:
         return database::create_node_fn(sched::direct_local::create_node);
      case SCHED_MPI_AND_THREADS_STATIC_LOCAL:
         return database::create_node_fn(sched::mpi_thread_static::create_node);
      case SCHED_MPI_AND_THREADS_DYNAMIC_LOCAL:
         return database::create_node_fn(sched::mpi_thread_dynamic::create_node);
      case SCHED_MPI_AND_THREADS_SINGLE_LOCAL:
         return database::create_node_fn(sched::mpi_thread_single::create_node);
      case SCHED_SERIAL_LOCAL:
         return database::create_node_fn(sched::serial_local::create_node);
		case SCHED_SERIAL_UI_LOCAL:
			return database::create_node_fn(sched::serial_ui_local::create_node);
      case SCHED_UNKNOWN:
         return NULL;
   }
   
   throw machine_error("unknown scheduler type");
}

machine::machine(const string& file, router& _rout, const size_t th,
		const scheduler_type _sched_type, const machine_arguments& margs):
   filename(file),
   num_threads(th),
   sched_type(_sched_type),
   rout(_rout),
   alarm_thread(NULL),
   slices(th)
{  
   new program(filename);

	if(margs.size() < state::PROGRAM->num_args_needed())
		throw machine_error(string("this program requires ") + to_string(state::PROGRAM->num_args_needed()) + " arguments");
   
	state::ARGUMENTS = margs;
   state::DATABASE = new database(filename, get_creation_function(_sched_type));
   state::NUM_THREADS = num_threads;
   state::MACHINE = this;
   
   mem::init(num_threads);
   process_list.resize(num_threads);
   
   vector<sched::base*> schedulers;
   
   switch(sched_type) {
      case SCHED_THREADS_STATIC_LOCAL:
         schedulers = sched::static_local::start(num_threads);
         break;
      case SCHED_THREADS_STATIC_LOCAL_PRIO:
         schedulers = sched::static_local_prio::start(num_threads);
         break;
      case SCHED_THREADS_SINGLE_LOCAL:
         schedulers = sched::threads_single::start(num_threads);
         break;
      case SCHED_THREADS_DYNAMIC_LOCAL:
         schedulers = sched::dynamic_local::start(num_threads);
         break;
      case SCHED_THREADS_DIRECT_LOCAL:
         schedulers = sched::direct_local::start(num_threads);
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
      case SCHED_SERIAL_LOCAL:
         schedulers.push_back(dynamic_cast<sched::base*>(new sched::serial_local()));
         break;
		case SCHED_SERIAL_UI_LOCAL:
			schedulers.push_back(dynamic_cast<sched::base*>(new sched::serial_ui_local()));
			break;
      case SCHED_UNKNOWN: assert(false); break;
   }
   
   assert(schedulers.size() == num_threads);
   
   for(process_id i(0); i < num_threads; ++i)
      process_list[i] = new process(i, schedulers[i]);
}

machine::~machine(void)
{
   // when deleting database, we need to access the program,
   // so we must delete this in correct order
   delete state::DATABASE;
   
   for(process_id i(0); i != num_threads; ++i)
      delete process_list[i];

   delete state::PROGRAM;
      
   if(alarm_thread)
      delete alarm_thread;
      
   mem::cleanup(num_threads);

   state::PROGRAM = NULL;
   state::DATABASE = NULL;
   state::MACHINE = NULL;
   state::REMOTE = NULL;
   state::ROUTER = NULL;
   state::NUM_THREADS = 0;
   state::NUM_PREDICATES = 0;
   state::NUM_NODES = 0;
   state::NUM_NODES_PER_PROCESS = 0;
}

}
