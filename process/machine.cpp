#include <iostream>
#include <signal.h>
#include <unistd.h>

#include "process/machine.hpp"
#include "vm/program.hpp"
#include "vm/state.hpp"
#include "vm/exec.hpp"
#include "mem/thread.hpp"
#include "mem/stat.hpp"
#include "stat/stat.hpp"
#include "utils/fs.hpp"
#include "interface.hpp"
#ifdef USE_UI
#include "sched/serial_ui.hpp"
#include "ui/manager.hpp"
#endif
#ifdef USE_SIM
#include "sched/sim.hpp"
#endif
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
machine::run_action(sched::base *sched, node* node, vm::tuple *tpl, vm::predicate *pred)
{
	const predicate_id pid(pred->get_id());
	
	assert(pred->is_action_pred());
	
   switch(pid) {
      case SETCOLOR_PREDICATE_ID:
#ifdef USE_UI
      if(state::UI) {
         LOG_SET_COLOR(node, tpl->get_int(0), tpl->get_int(1), tpl->get_int(2));
      }
#endif
#ifdef USE_SIM
      if(state::SIM) {
			((sim_sched*)sched)->set_color(node, tpl->get_int(0), tpl->get_int(1), tpl->get_int(2));
      }
#endif
      break;
      case SETCOLOR2_PREDICATE_ID:
#ifdef USE_SIM
      if(state::SIM) {
         int r(0), g(0), b(0);
         switch(tpl->get_int(0)) {
            case 0: r = 255; break; // RED
            case 1: r = 255; g = 160; break; // ORANGE
            case 2: r = 255; g = 247; break; // YELLOW
            case 3: g = 255; break; // GREEN
            case 4: g = 191; b = 255; break; // AQUA
            case 5: b = 255; break; // BLUE
            case 6: r = 255; g = 255; b = 255; break; // WHITE
            case 7: r = 139; b = 204; break; // PURPLE
            case 8: r = 255; g = 192; b = 203; break; // PINK
            case -1: return; break;
            default: break;
         }
			((sim_sched*)sched)->set_color(node, r, g, b);
      }
#endif
      break;
      case SETEDGELABEL_PREDICATE_ID:
#ifdef USE_UI
      if(state::UI) {
         LOG_SET_EDGE_LABEL(node->get_id(), tpl->get_node(0), tpl->get_string(1)->get_content());
      }
#endif
      break;
      case WRITE_STRING_PREDICATE_ID: {
         runtime::rstring::ptr s(tpl->get_string(0));

         cout << s->get_content() << endl;
        }
      break;
      case SCHEDULE_NEXT_PREDICATE_ID:
         sched->schedule_next(node);
      break;
      default:
		assert(false);
      break;
   }

   vm::tuple::destroy(tpl, pred);
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

void
machine::execute_const_code(void)
{
	state st;
	
	// no node or tuple whatsoever
	st.setup(NULL, NULL, 0, 0);
	
	execute_process(all->PROGRAM->get_const_bytecode(), st, NULL, NULL);
}

void
machine::init_sched(const process_id id)
{
   // ensure own memory pool
   mem::ensure_pool();

   switch(sched_type) {
      case SCHED_THREADS:
         all->SCHEDS[id] = dynamic_cast<sched::base*>(new sched::threads_sched(id));
         break;
#ifdef USE_UI
      case SCHED_SERIAL_UI:
         all->SCHEDS[id] = dynamic_cast<sched::base*>(new sched::serial_ui_local());
         break;
#endif
#ifdef USE_SIM
      case SCHED_SIM:
         all->SCHEDS[id] = dynamic_cast<sched::base*>(new sched::sim_sched());
         break;
#endif
      case SCHED_UNKNOWN: assert(false); break;
   }

   all->SCHEDS[id]->loop();
}

void
machine::start(void)
{
	// execute constants code
	execute_const_code();
	
   deactivate_signals();
   
   if(stat_enabled()) {
      // initiate alarm thread
      alarm_thread = new thread(bind(&machine::slice_function, this));
   }

   switch(sched_type) {
      case SCHED_THREADS:
         sched::threads_sched::init_barriers(all->NUM_THREADS);
         break;
      default: break;
   }
   
   thread *threads[all->NUM_THREADS];
   for(process_id i(1); i < all->NUM_THREADS; ++i) {
      threads[i] = new thread(&machine::init_sched, this, i);
   }
   init_sched(0);
   
   for(size_t i(1); i < all->NUM_THREADS; ++i) {
      threads[i]->join();
   }

#if 0
   cout << "Total Facts: " << this->all->DATABASE->total_facts() << endl;
   cout << "Total Nodes: " << this->all->DATABASE->num_nodes() << endl;
   cout << "Bytes used: " << bytes_used << endl;
#endif
#ifdef LOCK_STATISTICS
   utils::mutex::print_statistics();
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
      
   if(alarm_thread) {
      kill(getpid(), SIGUSR1);
      alarm_thread->join();
      delete alarm_thread;
      alarm_thread = NULL;
      slices.write(get_stat_file(), sched_type, all);
   }

   const bool will_print(show_database || dump_database);

   if(will_print) {
      if(show_database)
         all->DATABASE->print_db(cout);
      if(dump_database)
         all->DATABASE->dump_db(cout);
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
      case SCHED_THREADS:
         return database::create_node_fn(sched::threads_sched::create_node);
#ifdef USE_UI
		case SCHED_SERIAL_UI:
			return database::create_node_fn(sched::serial_ui_local::create_node);
#endif
#ifdef USE_SIM
		case SCHED_SIM:
			return database::create_node_fn(sched::sim_sched::create_node);
#endif
      case SCHED_UNKNOWN:
         return NULL;
   }
   
   throw machine_error("unknown scheduler type");
}

machine::machine(const string& file, router& _rout, const size_t th,
		const scheduler_type _sched_type, const machine_arguments& margs, const string& data_file):
   all(new vm::all()),
   filename(file),
   sched_type(_sched_type),
   rout(_rout),
   alarm_thread(NULL),
   slices(th)
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

   this->all->ROUTER = &_rout;

   if(margs.size() < this->all->PROGRAM->num_args_needed())
      throw machine_error(string("this program requires ") + utils::to_string(all->PROGRAM->num_args_needed()) + " arguments");

   this->all->set_arguments(margs);
   this->all->DATABASE = new database(added_data_file ? data_file : filename, get_creation_function(_sched_type));
   this->all->NUM_THREADS = th;
   this->all->MACHINE = this;
   this->all->SCHEDS.resize(th, NULL);
}

machine::~machine(void)
{
   // when deleting database, we need to access the program,
   // so we must delete this in correct order
   delete this->all->DATABASE;
   
   for(process_id i(0); i != all->NUM_THREADS; ++i)
      delete all->SCHEDS[i];

   delete this->all->PROGRAM;
      
   if(alarm_thread)
      delete alarm_thread;
}

}
