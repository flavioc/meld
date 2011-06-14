
#include <fstream>
#include <sys/stat.h>
#include <iostream>

#include "vm/program.hpp"
#include "db/tuple.hpp"
#include "vm/instr.hpp"
#include "db/database.hpp"
#include "utils/types.hpp"
#include "vm/state.hpp"

using namespace std;
using namespace db;
using namespace vm;
using namespace vm::instr;
using namespace process;
using namespace utils;

namespace vm {

static const size_t PREDICATE_DESCRIPTOR_SIZE = sizeof(code_size_t) +
                                                PRED_DESCRIPTOR_BASE_SIZE +
                                                PRED_ARGS_MAX +
                                                PRED_NAME_SIZE_MAX +
                                                PRED_AGG_INFO_MAX;

program::program(const string& filename)
{
   state::PROGRAM = this;
   
   ifstream fp(filename.c_str(), ios::in | ios::binary);
   
   if(!fp.is_open())
      throw load_file_error(filename);

   // read number of predicates
   byte buf[PREDICATE_DESCRIPTOR_SIZE];
   
   fp.read((char*)buf, sizeof(byte));
   
   const size_t num_predicates = (size_t)buf[0];
   
   predicates.resize(num_predicates);
   code_size.resize(num_predicates);
   code.resize(num_predicates);
   state::NUM_PREDICATES = num_predicates;
   
   // skip nodes
   int_val num_nodes;
   fp.read((char*)&num_nodes, sizeof(int_val));
   
   fp.seekg(num_nodes * database::node_size, ios_base::cur);
   
   // read predicate information
   for(size_t i(0); i < num_predicates; ++i) {
      code_size_t size;
      fp.read((char*)buf, PREDICATE_DESCRIPTOR_SIZE);
      
      predicates[i] = predicate::make_predicate_from_buf((unsigned char*)buf, &size);
      code_size[i] = size;
      
      if(predicates[i]->is_route_pred())
         route_predicates.push_back(predicates[i]);
   }
   
   for(size_t i(0); i < num_predicates; ++i)
      predicates[i]->cache_info();
   
   // read predicate code
   for(size_t i(0); i < num_predicates; ++i) {
      const size_t size = code_size[i];
      code[i] = new byte_code_el[size];
      
      fp.read((char*)code[i], size);
   }
}

program::~program(void)
{
   for(size_t i(0); i < num_predicates(); ++i) {
      delete predicates[i];
      delete [] code[i];
   }
}

predicate*
program::get_predicate(const predicate_id& id) const
{
   if((size_t)id >= num_predicates()) {
      return NULL;
   }
   
   return predicates[id];
}

predicate*
program::get_route_predicate(const size_t& i) const
{
   assert(i < num_route_predicates());
   
   return route_predicates[i];
}

byte_code
program::get_bytecode(const predicate_id& id) const
{
   if((size_t)id >= num_predicates())
      return NULL;
      
   return code[id];
}

void
program::print_bytecode(ostream& cout) const
{
   for(size_t i = 0; i < num_predicates(); ++i) {
      predicate_id id = (predicate_id)i;
      
      if(i != 0)
         cout << endl;
         
      cout << "PROCESS " << get_predicate(id)->get_name()
           << " (" << code_size[i] << "):" << endl;
      instrs_print(code[i], code_size[i], this, cout);
      cout << "END PROCESS;" << endl;
   }
}

void
program::print_predicates(ostream& cout) const
{
   for(size_t i(0); i < num_predicates(); ++i) {
      cout << *predicates[i] << endl;
   }
}

predicate*
program::get_predicate_by_name(const string& name) const
{
   for(size_t i = 0; i < num_predicates(); ++i) {
      predicate *pred(get_predicate((predicate_id)i));
      
      if(pred->get_name() == name)
         return pred;
   }
   
   return NULL;
}

predicate*
program::get_init_predicate(void) const
{
   static predicate *init(NULL);
   
   if(init == NULL)
      init = get_predicate_by_name("_init");
      
   return init;
}

predicate*
program::get_edge_predicate(void) const
{
   return get_predicate_by_name("edge");
}

tuple*
program::new_tuple(const predicate_id& id) const
{
   return new tuple(get_predicate(id));
}

}