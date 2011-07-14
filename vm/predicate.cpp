#include <assert.h>
#include <iostream>

#include "vm/predicate.hpp"
#include "vm/state.hpp"

using namespace std;
using namespace vm;
using namespace utils;

namespace vm {

#define PRED_AGG 0x01
#define PRED_ROUTE 0x02
#define PRED_REVERSE_ROUTE 0x04

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
   pred->is_route = prop & PRED_ROUTE;
   pred->is_reverse_route = prop & PRED_REVERSE_ROUTE;
   buf++;
   
   // get aggregate information, if any
   if(pred->is_aggregate()) {
      unsigned char agg = buf[0];
      
      pred->agg_info->field = agg & 0xf;
      pred->agg_info->type = (aggregate_type)((0xf0 & agg) >> 4);
      pred->agg_info->with_remote_pred = false;
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
   buf += PRED_ARGS_MAX;
   
   // read predicate name
   pred->name = string((const char*)buf);
   
   buf += PRED_NAME_SIZE_MAX;
   
   if(pred->is_aggregate()) {
      const size_t local_total(buf[0]);
      
      buf++;
      
      for(size_t i(0); i < local_total; ++i) {
         predicate_id id(buf[0]);
         pred->agg_info->local_sizes.push_back(id);  
         buf++;
      }
      
      const size_t remote_total(buf[0]);
      
      buf++;
      
      assert(remote_total <= 1);
      if(remote_total == 1) {
         pred->agg_info->with_remote_pred = true;
         pred->agg_info->remote_pred_id = (predicate_id)(buf[0]);
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

void
predicate::build_aggregate_info(void)
{
   for(size_t i(0); i < agg_info->local_sizes.size(); ++i) {
      const predicate_id id(agg_info->local_sizes[i]);
      const predicate *pred(state::PROGRAM->get_predicate(id));
      
      agg_info->local_predicates.push_back(pred);
   }
   
   if(agg_info->with_remote_pred) {
      agg_info->remote_pred = state::PROGRAM->get_predicate(agg_info->remote_pred_id);
   }
}

void
predicate::cache_info(void)
{
   build_field_info();
   if(is_aggregate())
      build_aggregate_info();
}

const vector<const predicate*>&
predicate::get_local_agg_deps(void) const
{
   assert(is_aggregate());
   
   return agg_info->local_predicates;
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
   
   if(is_route)
      cout << ",route";
   if(is_reverse_route)
      cout << ",reverse_route";
   
   cout << "]";
   
   if(is_aggregate()) {
      const vector<const predicate*>& locals(get_local_agg_deps());
      if(!locals.empty()) {
         cout << "[local_deps=";
         for(size_t i(0); i < locals.size(); ++i) {
            if(i != 0)
               cout << ",";
            cout << locals[i]->get_name();
         }
         cout << "]";
      }
      if(agg_info->with_remote_pred)
         cout << "[remote_dep=" << agg_info->remote_pred->get_name() << "]";
   }
}

ostream& operator<<(ostream& cout, const predicate& pred)
{
   pred.print(cout);
   return cout;
}

}