#include <assert.h>
#include <iostream>

#include "vm/predicate.hpp"

using namespace std;
using namespace vm;

namespace vm {

#define PRED_AGG 0x01

predicate_id predicate::current_id = 0;

predicate*
predicate::make_predicate_from_buf(unsigned char *buf, size_t *code_size)
{
   predicate *pred = new predicate();
   
   pred->id = current_id++;

   // get code size
   *code_size = (size_t)*((unsigned short*)buf);
   buf += sizeof(unsigned short);
   
   // get predicate properties
   unsigned char prop = buf[0];
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
      
      cout << field_type_string(types[i]);
      
      if(is_aggregate() && agg_info->field == i) {
         cout << aggregate_type_string(agg_info->type);
      }
   }
   
   cout << ") [size=" << tuple_size;
   
   if(is_aggregate())
      cout << ",agg";
   
   cout << "]";
}

ostream& operator<<(ostream& cout, const predicate& pred)
{
   pred.print(cout);
   return cout;
}

}