#include <iostream>
#include <vector>

#include "process/exec.hpp"
#include "vm/program.hpp"

using namespace process;
using namespace db;
using namespace std;
using namespace vm;

namespace process
{

static void
distribute_nodes_over_processes(vector<process*> process_list, database *db)
{
   const size_t total(db->num_nodes());
   const size_t num_procs(process_list.size());
   const size_t nodes_per_proc((total + num_procs/2) / num_procs);
   size_t num_nodes = nodes_per_proc;
   size_t cur_proc = 0;
   
   for(database::map_nodes::const_iterator it(db->nodes_begin());
         it != db->nodes_end();
         it++)
   {
      if(num_nodes == 0) {
         num_nodes = nodes_per_proc;
         ++cur_proc;
      }
      
      process_list[cur_proc]->add_node(it->second);
      
      --num_nodes;
   }
}

void
execute_file(const string& filename, const size_t num_threads)
{
   process::PROGRAM = new program(filename);
   process::DATABASE = process::PROGRAM->get_database();
   
   //process::PROGRAM->print_byte_code(cout);
   
   vector<process*> process_list;
   process_list.resize(num_threads);
   
   for(size_t i = 0; i < num_threads; ++i) {
      process_list[i] = new process((process_id)i);
   }

   distribute_nodes_over_processes(process_list, process::DATABASE);
   
   for(size_t i = 0; i < num_threads; ++i) {
      process_list[i]->start();
   }
}

}
