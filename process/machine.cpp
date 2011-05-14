#include <iostream>

#include "process/machine.hpp"
#include "vm/program.hpp"
#include "vm/state.hpp"
#include "process/process.hpp"
#include "runtime/list.hpp"
#include "process/message.hpp"
#include "mem/thread.hpp"

using namespace process;
using namespace db;
using namespace std;
using namespace vm;
using namespace boost;

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
machine::route(process *caller, const node::node_id id, const simple_tuple* stuple)
{  
   remote* rem(rout.find_remote(id));
   sched::base *sched_caller(caller->get_scheduler());
   
   assert(id <= state::DATABASE->max_id());
   
   if(rem == remote::self) {
      // on this machine
      
      node *node(state::DATABASE->find_node(id));
      
      sched::base *sched_other(sched_caller->find_scheduler(id));
      
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

void
machine::start(void)
{
   for(size_t i(1); i < num_threads; ++i)
      process_list[i]->start();
   process_list[0]->start();
   
   for(size_t i(1); i < num_threads; ++i)
      process_list[i]->join();

   if(will_show_database)
      state::DATABASE->print_db(cout);
   if(will_dump_database)
      state::DATABASE->dump_db(cout);
}

static inline database::create_node_fn
get_creation_function(const scheduler_type sched_type)
{
   switch(sched_type) {
      case SCHED_THREADS_STATIC_GLOBAL:
      case SCHED_MPI_UNI_STATIC:
         return database::create_node_fn(sched::sstatic::create_node);
      case SCHED_THREADS_STATIC_LOCAL:
         return database::create_node_fn(sched::static_local::create_node);
      case SCHED_THREADS_DYNAMIC_LOCAL:
         return database::create_node_fn(sched::dynamic_local::create_node);
      case SCHED_MPI_AND_THREADS_DYNAMIC_LOCAL:
         return database::create_node_fn(sched::mpi_thread::create_node);
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
   rout(_rout)
{  
   state::PROGRAM = new program(filename);
   state::DATABASE = new database(filename, get_creation_function(_sched_type));
   state::NUM_THREADS = num_threads;
   state::MACHINE = this;
   
   printf("MAX STRAT %d\n", predicate::MAX_STRAT_LEVEL);
   
   mem::init(num_threads);
   process_list.resize(num_threads);
   
   switch(sched_type) {
      case SCHED_THREADS_STATIC_GLOBAL: {
            vector<sched::base*> schedulers(sched::static_global::start(num_threads));
      
            for(process_id i(0); i < num_threads; ++i)
               process_list[i] = new process(i, schedulers[i]);
         }
         break;
      case SCHED_MPI_UNI_STATIC:
#ifdef COMPILE_MPI
         process_list[0] = new process(0, sched::mpi_static::start());
#endif
         break;
      case SCHED_THREADS_STATIC_LOCAL: {
            vector<sched::base*> schedulers(sched::static_local::start(num_threads));
            
            for(process_id i(0); i < num_threads; ++i)
               process_list[i] = new process(i, schedulers[i]);
         }
         break;
      case SCHED_THREADS_DYNAMIC_LOCAL: {
            vector<sched::base*> schedulers(sched::dynamic_local::start(num_threads));
            for(process_id i(0); i < num_threads; ++i)
               process_list[i] = new process(i, schedulers[i]);
         }
         break;
      case SCHED_MPI_AND_THREADS_DYNAMIC_LOCAL: {
            vector<sched::base*> schedulers(sched::mpi_thread::start(num_threads));
            for(process_id i(0); i < num_threads; ++i)
               process_list[i] = new process(i, schedulers[i]);
         }
         break;
      case SCHED_UNKNOWN:
         assert(0);
         break;
   }
}

machine::~machine(void)
{
   delete state::PROGRAM;
   delete state::DATABASE;
   
   for(process_id i(0); i != num_threads; ++i)
      delete process_list[i];
      
   mem::cleanup(num_threads);
}

}
