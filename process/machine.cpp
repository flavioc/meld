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

void
machine::route(process *caller, const node::node_id id, const simple_tuple* stuple)
{  
   remote* rem(rout.find_remote(id));
   
   if(rem == remote::self) {
      // on this machine
      node *node(state::DATABASE->find_node(id));
      process *proc(process_list[remote::self->find_proc_owner(id)]);
      if(caller == proc)
         caller->enqueue_work(node, stuple);
      else
         caller->enqueue_other(proc, node, stuple);
   } else {
      // remote, mpi machine
      
      assert(rout.use_mpi());
      
      printf("On remote machine: %d\n", id);
      
      message msg(id, stuple);
      
      rout.send(rem, rem->find_proc_owner(id), msg);
      
      delete stuple;
   }
}

void
machine::distribute_nodes(database *db)
{
   const size_t total(remote::self->get_total_nodes());
   const size_t num_procs(process_list.size());
   
   if(total < num_procs)
      throw process_exec_error("Number of nodes is less than the number of threads");
   
   size_t nodes_per_proc = remote::self->get_nodes_per_proc();
   size_t num_nodes = nodes_per_proc;
   size_t cur_proc = 0;
   database::map_nodes::const_iterator it(db->nodes_begin());
	database::map_nodes::const_iterator end(db->nodes_end());
   
   for(; it != end && cur_proc < num_procs;
         ++it)
   {  
      process_list[cur_proc]->add_node(it->second);
   
      --num_nodes;
   
      if(num_nodes == 0) {
         num_nodes = nodes_per_proc;
         ++cur_proc;
      }
   }
   
   if(it != end) {
      // put remaining nodes on last process
      --cur_proc;
      
      for(; it != end; ++it)
         process_list[cur_proc]->add_node(it->second);
   }
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

void
machine::process_is_active(void)
{
   if(__sync_fetch_and_add(&threads_active, 1) == 1)
      state::ROUTER->update_status(router::REMOTE_ACTIVE);
}

void
machine::process_is_inactive(void)
{
   if(__sync_fetch_and_sub(&threads_active, 1) == 0)
      state::ROUTER->update_status(router::REMOTE_IDLE);
}

bool
machine::all_ended(void)
{
   for(size_t i(0); i < num_threads; ++i)
      if(!process_list[i]->has_ended())
         return false;
   return true;
}

machine::machine(const string& file, router& _rout, const size_t th):
   filename(file), num_threads(th), will_show_database(false),
   will_dump_database(false),
   rout(_rout), proc_barrier(new barrier(th)),
   threads_active(th), is_finished(false)
{  
   state::PROGRAM = new program(filename, &rout);
   state::DATABASE = state::PROGRAM->get_database();
   state::MACHINE = this;
   
   process_list.resize(num_threads);
   
   for(process::process_id i(0); i < num_threads; ++i)
      process_list[i] = new process(i, num_threads);
   
   mem::init(num_threads);
   
   distribute_nodes(state::DATABASE);
}

machine::~machine(void)
{
   delete state::PROGRAM;
   delete proc_barrier;
   
   for(process::process_id i(0); i != num_threads; ++i)
      delete process_list[i];
      
   mem::cleanup(num_threads);
}

}
