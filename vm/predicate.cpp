#include <assert.h>
#include <iostream>

#include "vm/predicate.hpp"
#include "vm/state.hpp"

using namespace std;
using namespace vm;
using namespace utils;

namespace vm {

#define PRED_AGG 0x01

predicate_id predicate::current_id = 0;
strat_level predicate::MAX_STRAT_LEVEL = 0;

predicate*
predicate::make_predicate_from_buf(byte *buf, code_size_t *code_size)
{
   predicate *pred = new predicate();
   
   pred->id = current_id++;

   // get code size
   *code_size = (code_size_t)*((code_size_t*)buf);
   buf += sizeof(code_size_t);
   
   // get predicate properties
   byte prop = buf[0];
   if(prop & PRED_AGG)
      pred->agg_info = new predicate::aggregate_info;
   else
      pred->agg_info = NULL;
   buf++;
   
   // get aggregate information, if any
   if(pred->is_aggregate()) {
      unsigned char agg = buf[0];
      
      pred->agg_info->field = agg & 0xf;
      pred->agg_info->type = (aggregate_type)((0xf0 & agg) >> 4);
   }
   buf++;
   
   // read stratification level
   pred->level = (strat_level)buf[0];
   MAX_STRAT_LEVEL = max(pred->level + 1, MAX_STRAT_LEVEL);
   buf++;

   // read number of fields
   pred->types.resize((size_t)buf[0]);
   buf++;
   
   // read argument types
   for(size_t i = 0; i < pred->num_fields(); ++i)
      pred->types[i] = (field_type)buf[i];
   pred->build_field_info();
   buf += PRED_ARGS_MAX;
   
   // read predicate name
   pred->name = string((const char*)buf);
   
   buf += PRED_NAME_SIZE_MAX;
   
   if(pred->is_aggregate()) {
      const size_t total(buf[0]);
      
      buf++;
      
      for(size_t i(0); i < total; ++i) {
         predicate_id id(buf[0]);
         
         pred->agg_info->sizes.push_back(id);
         
         buf++;
      }
   }
   
   return pred;
}

void
predicate::build_field_info(void)
{
   size_t offset = 0;
   
   fields_size.resize(num_fields());
   fields_offset.resize(num_fields());
   
   for(size_t i = 0; i < num_fields(); ++i) {
      const size_t size = field_type_size(types[i]);
      
      fields_size[i] = size;
      fields_offset[i] = offset;
      offset += size;
   }
   
   tuple_size = offset;
}

vector<const predicate*>
predicate::get_agg_deps(void) const
{
   assert(is_aggregate());
   
   vector<const predicate*> ret;
   
   for(size_t i(0); i < agg_info->sizes.size(); ++i) {
      const predicate_id id(agg_info->sizes[0]);
      const predicate *pred(state::PROGRAM->get_predicate(id));
      
      ret.push_back(pred);
   }
   
   return ret;
}

predicate::predicate(void)
{
   tuple_size = 0;
   agg_info = NULL;
}

void
predicate::print(ostream& cout) const
{
   cout << name << "(";
   
   for(size_t i = 0; i < num_fields(); ++i) {
      if(i != 0)
         cout << ", ";

      const string typ(field_type_string(types[i]));
      
      if(is_aggregate() && agg_info->field == i)
         cout << aggregate_type_string(agg_info->type) << " " << typ;
      else
         cout << typ;
   }
   
   cout << ") [size=" << tuple_size;
   
   if(is_aggregate())
      cout << ",agg";
      
   cout << ",strat_level=" << get_strat_level();
   
   cout << "]";
}

ostream& operator<<(ostream& cout, const predicate& pred)
{
   pred.print(cout);
   return cout;
}

}