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
#define PRED_REUSED 0x20
#define PRED_CYCLE 0x40
#define PRED_THREAD 0x80
#define PRED_COMPACT 0x01
#define PRED_AGG_LOCAL 0x01
#define PRED_AGG_REMOTE 0x02
#define PRED_AGG_REMOTE_AND_SELF 0x04
#define PRED_AGG_IMMEDIATE 0x08
#define PRED_AGG_UNSAFE 0x00

type* read_type_from_reader(code_reader& read, vm::program* prog) {
   byte f;
   read.read_type<byte>(&f);
   const field_type t((field_type)f);

   switch (t) {
      case FIELD_BOOL:
      case FIELD_INT:
      case FIELD_FLOAT:
      case FIELD_NODE:
      case FIELD_STRING:
      case FIELD_THREAD:
         return new type(t);
      case FIELD_LIST: {
         byte id;
         read.read_type<byte>(&id);
         return new list_type(prog->get_type(id));
      }
      case FIELD_STRUCT: {
         byte size;
         byte id;
         read.read_type<byte>(&size);
         auto ret = new struct_type((size_t)size);
         for (size_t i(0); i < ret->get_size(); ++i) {
            read.read_type<byte>(&id);
            ret->set_type(i, prog->get_type(id));
         }
         return ret;
      }
      case FIELD_ARRAY: {
         byte id;
         read.read_type<byte>(&id);
         return new array_type(prog->get_type(id));
      }
      case FIELD_SET: {
         byte id;
         read.read_type<byte>(&id);
         return new set_type(prog->get_type(id));
      }
      default:
         assert(false);
         abort();
         break;
   }
   assert(false);
   return nullptr;
}

type* read_type_id_from_reader(
    code_reader& read, const vector<type*, mem::allocator<type*>>& types) {
   byte p;
   read.read_type<byte>(&p);
   const size_t pos((size_t)p);
   assert(pos < types.size());

   return types[pos];
}

predicate* predicate::make_predicate_simple(const predicate_id id,
                                            const string& name,
                                            const bool linear,
                                            const vector<type*>& types) {
   auto pred = new predicate();

   pred->id = id;

   pred->agg_info = nullptr;
   pred->is_linear = linear;
   pred->types = types;
   pred->name = name;

   return pred;
}

predicate* predicate::make_predicate_from_reader(
    code_reader& read, code_size_t* code_size, const predicate_id id,
    const uint32_t major_version, const uint32_t minor_version,
    const vector<type*, mem::allocator<type*>>& types) {
   auto pred = new predicate();

   pred->id = id;

   // get code size
   read.read_type<code_size_t>(code_size);

   // get predicate properties
   byte prop;
   read.read_type<byte>(&prop);
   if (prop & PRED_AGG)
      pred->agg_info = new predicate::aggregate_info;
   else
      pred->agg_info = nullptr;
   pred->is_linear = prop & PRED_LINEAR;
   pred->is_route = prop & PRED_ROUTE;
   pred->is_reverse_route = prop & PRED_REVERSE_ROUTE;
   pred->is_action = prop & PRED_ACTION;
   pred->is_reused = prop & PRED_REUSED;
   pred->is_cycle = prop & PRED_CYCLE;
   pred->is_thread = prop & PRED_THREAD;

   if(VERSION_AT_LEAST(0, 13)) {
      read.read_type<byte>(&prop);
      if(prop & PRED_COMPACT)
         pred->is_compact = true;
   }

   // get aggregate information, if any
   byte agg;
   read.read_type<byte>(&agg);
   if (pred->is_aggregate_pred()) {
      pred->agg_info->safeness = AGG_UNSAFE;
      pred->agg_info->field = agg & 0xf;
      pred->agg_info->type = (aggregate_type)((0xf0 & agg) >> 4);
   }

   // read stratification level
   byte level;
   read.read_type<byte>(&level);
   pred->level = (strat_level)level;

   if (VERSION_AT_LEAST(0, 12)) {
      byte field_index;
      read.read_type<byte>(&field_index);
      if (field_index != 0)
         pred->store_as_hash_table(field_index - 1);
   }

   // read number of fields
   byte nfields;
   read.read_type<byte>(&nfields);
   pred->types.resize((size_t)nfields);

   // read argument types
   if (VERSION_AT_LEAST(0, 10)) {
      for (size_t i(0); i < pred->num_fields(); ++i)
         pred->types[i] = read_type_id_from_reader(read, types);
   } else {
      for (size_t i = 0; i < PRED_ARGS_MAX; ++i) {
         byte f;
         read.read_type<byte>(&f);
         if (i < pred->num_fields()) {
            switch (f) {
               case 0x3:
                  pred->types[i] = new list_type(new type(FIELD_INT));
                  break;
               case 0x4:
                  pred->types[i] = new list_type(new type(FIELD_FLOAT));
                  break;
               case 0x5:
                  pred->types[i] = new list_type(new type(FIELD_NODE));
                  break;
               default:
                  pred->types[i] = new type((field_type)f);
                  break;
            }
         }
      }
   }

   // read predicate name
   char name_buf[PRED_NAME_SIZE_MAX];
   read.read_any(name_buf, PRED_NAME_SIZE_MAX);
   pred->name = string((const char*)name_buf);

   char buf_vec[PRED_AGG_INFO_MAX];
   read.read_any(buf_vec, PRED_AGG_INFO_MAX);
   char* buf = buf_vec;

   if (pred->is_aggregate_pred()) {
      if (buf[0] == PRED_AGG_LOCAL) {
         buf++;
         pred->agg_info->safeness = AGG_LOCALLY_GENERATED;
         pred->agg_info->local_level = (strat_level)(buf[0]);
      } else if (buf[0] == PRED_AGG_REMOTE) {
         buf++;
         pred->agg_info->safeness = AGG_NEIGHBORHOOD;
         pred->agg_info->remote_pred_id = (predicate_id)(buf[0]);
      } else if (buf[0] == PRED_AGG_REMOTE_AND_SELF) {
         buf++;
         pred->agg_info->safeness = AGG_NEIGHBORHOOD_AND_SELF;
         pred->agg_info->remote_pred_id = (predicate_id)(buf[0]);
      } else if (buf[0] == PRED_AGG_IMMEDIATE) {
         buf++;
         pred->agg_info->safeness = AGG_IMMEDIATE;
      } else if (buf[0] & PRED_AGG_UNSAFE) {
         buf++;
         pred->agg_info->safeness = AGG_UNSAFE;
      }
   }

   return pred;
}

void predicate::build_field_info(void) {
   size_t offset = 0;

   fields_size.resize(num_fields());
   fields_offset.resize(num_fields());

   for (size_t i = 0; i < num_fields(); ++i) {
      const size_t size = types[i]->size();

      fields_size[i] = size;
      fields_offset[i] = offset;
      offset += size;
   }

   tuple_size = offset;
}

void predicate::build_aggregate_info(vm::program* prog) {
   switch (agg_info->safeness) {
      case AGG_NEIGHBORHOOD:
      case AGG_NEIGHBORHOOD_AND_SELF:
         agg_info->remote_pred = prog->get_predicate(agg_info->remote_pred_id);
         break;
      default:
         break;
   }
}

void predicate::cache_info(vm::program* prog) {
   build_field_info();
   if (is_aggregate_pred()) build_aggregate_info(prog);
}

predicate::predicate(void) : store_type(LINKED_LIST) {
   tuple_size = 0;
   agg_info = nullptr;
}

void predicate::destroy(void) { delete agg_info; }

predicate::~predicate(void) {
   // types are not deleted here, they are deleted in vm/program.
}

void predicate::print_simple(ostream& cout) const {
   if (is_persistent_pred()) cout << "!";

   cout << name << "(";

   for (size_t i = 0; i < num_fields(); ++i) {
      if (i != 0) cout << ", ";

      const string typ(types[i]->string());

      if (is_aggregate_pred() && agg_info->field == i)
         cout << aggregate_type_string(agg_info->type) << " " << typ;
      else
         cout << typ;
   }

   cout << ")/" << num_fields();
}

void predicate::print(ostream& cout) const {
   print_simple(cout);

   cout << " [size=" << tuple_size;

   if (is_aggregate_pred()) cout << ",agg";

   cout << ",strat_level=" << get_strat_level();

   if (is_route) cout << ",route";
   if (is_reverse_route) cout << ",reverse_route";
   if (is_linear) cout << ",linear";
   if (is_action) cout << ",action";
   if (is_reused) cout << ",reused";
   if (is_cycle) cout << ",cycle";
   if (is_thread) cout << ",thread";
   if (is_compact) cout << ",compact";

   cout << "]";

   if (is_aggregate_pred()) {
      cout << "[";
      switch (get_agg_safeness()) {
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
         default:
            assert(false);
            break;
      }
      cout << "]";
   }
}

bool predicate::operator==(const predicate& other) const {
   if (id != other.id) return false;

   if (num_fields() != other.num_fields()) return false;

   if (name != other.name) return false;

   for (size_t i = 0; i < num_fields(); ++i) {
      if (types[i] != other.types[i]) return false;
   }

   return true;
}

ostream& operator<<(ostream& cout, const predicate& pred) {
   pred.print(cout);
   return cout;
}
}
