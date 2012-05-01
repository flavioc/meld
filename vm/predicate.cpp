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
#define PRED_LINEAR 0x08
#define PRED_ACTION 0x10
#define PRED_AGG_LOCAL 0x01
#define PRED_AGG_REMOTE 0x02
#define PRED_AGG_REMOTE_AND_SELF 0x04
#define PRED_AGG_IMMEDIATE 0x08
#define PRED_AGG_UNSAFE 0x00

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
   pred->is_linear = prop & PRED_LINEAR;
   pred->is_route = prop & PRED_ROUTE;
   pred->is_reverse_route = prop & PRED_REVERSE_ROUTE;
   pred->is_action = prop & PRED_ACTION;
   buf++;

   if(pred->is_action)
      assert(pred->is_linear); // must also be linear
   
   // get aggregate information, if any
   if(pred->is_aggregate()) {
      unsigned char agg = buf[0];
      
      pred->agg_info->safeness = AGG_UNSAFE;
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
   buf += PRED_ARGS_MAX;
   
   // read predicate name
   pred->name = string((const char*)buf);
   
   buf += PRED_NAME_SIZE_MAX;
   
   if(pred->is_aggregate()) {
      if(buf[0] == PRED_AGG_LOCAL) {
         buf++;
         pred->agg_info->safeness = AGG_LOCALLY_GENERATED;
         pred->agg_info->local_level = (strat_level)(buf[0]);
      } else if(buf[0] == PRED_AGG_REMOTE) {
         buf++;
         pred->agg_info->safeness = AGG_NEIGHBORHOOD;
         pred->agg_info->remote_pred_id = (predicate_id)(buf[0]);
      } else if(buf[0] == PRED_AGG_REMOTE_AND_SELF) {
         buf++;
         pred->agg_info->safeness = AGG_NEIGHBORHOOD_AND_SELF;
         pred->agg_info->remote_pred_id = (predicate_id)(buf[0]);
      } else if(buf[0] == PRED_AGG_IMMEDIATE) {
         buf++;
         pred->agg_info->safeness = AGG_IMMEDIATE;
      } else if(buf[0] & PRED_AGG_UNSAFE) {
         buf++;
         pred->agg_info->safeness = AGG_UNSAFE;
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
   switch(agg_info->safeness) {
      case AGG_NEIGHBORHOOD:
      case AGG_NEIGHBORHOOD_AND_SELF:
         agg_info->remote_pred = state::PROGRAM->get_predicate(agg_info->remote_pred_id);
         break;
      default: break;
   }
}

void
predicate::cache_info(void)
{
   build_field_info();
   if(is_aggregate())
      build_aggregate_info();
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
   if(is_linear)
      cout << ",linear";
   if(is_action)
      cout << ",action";
   
   cout << "]";
   
   if(is_aggregate()) {
      cout << "[";
      switch(get_agg_safeness()) {
         case AGG_LOCALLY_GENERATED:
            cout << "local,local_level=" << get_agg_strat_level();
            break;
         case AGG_NEIGHBORHOOD:
            cout << "neighborhood=" << get_remote_pred()->get_name();
            break;
         case AGG_NEIGHBORHOOD_AND_SELF:
            cout << "neighborhood=" << get_remote_pred()->get_name() << ",self";
            break;
         case AGG_UNSAFE:
            cout << "unsafe";
            break;
         case AGG_IMMEDIATE:
            cout << "immediate";
            break;
         default: assert(false); break;
      }
      cout << "]";
   }
}

ostream& operator<<(ostream& cout, const predicate& pred)
{
   pred.print(cout);
   return cout;
}

}
