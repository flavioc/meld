#include <iostream>
#include <vector>

#include "process/exec.hpp"
#include "vm/program.hpp"
#include "vm/state.hpp"
#include "process/process.hpp"

using namespace process;
using namespace db;
using namespace std;
using namespace vm;
using namespace boost;

namespace process
{
   
process*
machine::find_owner(const node* node) const
{
   const node_id id(node->get_id());
   
   for(size_t i = 0; i < process_list.size(); ++i) {
      if(process_list[i]->owns_node(id))
         return process_list[i];
   }
   
   return NULL;
}

void
machine::distribute_nodes(database *db)
{
   const size_t total(db->num_nodes());
   const size_t num_procs(process_list.size());
   
   if(total < num_procs)
      throw process_exec_error("Number of nodes is less than the number of threads");
   
   const size_t nodes_per_proc(total / num_procs);
   size_t num_nodes = nodes_per_proc;
   size_t cur_proc = 0;
   auto it(db->nodes_begin());
   auto end(db->nodes_end());
   
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
   for(size_t i = 1; i < num_threads; ++i)
      process_list[i]->start();
   process_list[0]->start();
}

void
machine::process_is_active(void)
{
   mutex::scoped_lock lock(active_mutex);
   
   ++threads_active;
}

void
machine::process_is_inactive(void)
{
   mutex::scoped_lock lock(active_mutex);
   
   --threads_active;
   
   if(threads_active == 0) {
      cout << "PROGRAM ENDED" << endl;
      state::DATABASE->print_db(cout);
      exit(EXIT_SUCCESS);
   }
}

machine::machine(const string& file, const size_t th):
   filename(file), num_threads(th), threads_active(th)
{
   state::PROGRAM = new program(filename);
   state::DATABASE = state::PROGRAM->get_database();
   state::MACHINE = this;
   
   //state::PROGRAM->print_bytecode(cout);
   //exit(0);
   
   process_list.resize(num_threads);
   
   for(size_t i = 0; i < num_threads; ++i)
      process_list[i] = new process((process_id)i);

   distribute_nodes(state::DATABASE);
   
   for(size_t i = 0; i < num_threads; ++i)
      cout << *process_list[i] << endl;
}

}