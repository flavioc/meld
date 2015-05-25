
#include <cstdlib>
#include <assert.h>

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "utils/utils.hpp"

using namespace vm;
using namespace std;
using namespace utils;

namespace vm {

type *TYPE_INT(nullptr);
type *TYPE_FLOAT(nullptr);
type *TYPE_NODE(nullptr);
type *TYPE_THREAD(nullptr);
type *TYPE_STRING(nullptr);
type *TYPE_BOOL(nullptr);
type *TYPE_ANY(nullptr);
type *TYPE_STRUCT(nullptr);
type *TYPE_LIST(nullptr);
type *TYPE_ARRAY(nullptr);
type *TYPE_SET(nullptr);
type *TYPE_TYPE(nullptr);
list_type *TYPE_LIST_INT(nullptr);
list_type *TYPE_LIST_FLOAT(nullptr);
list_type *TYPE_LIST_NODE(nullptr);
list_type *TYPE_LIST_THREAD(nullptr);
array_type *TYPE_ARRAY_INT(nullptr);
static bool types_initiated(false);

void
init_types(void)
{
   if(types_initiated)
      return;

   types_initiated = true;

   TYPE_INT = new type(FIELD_INT);
   TYPE_FLOAT = new type(FIELD_FLOAT);
   TYPE_NODE = new type(FIELD_NODE);
   TYPE_THREAD = new type(FIELD_THREAD);
   TYPE_BOOL = new type(FIELD_BOOL);
   TYPE_STRING = new type(FIELD_STRING);
   TYPE_ANY = new type(FIELD_ANY);
   TYPE_STRUCT = new type(FIELD_STRUCT);
   TYPE_LIST = new type(FIELD_LIST);
   TYPE_ARRAY = new type(FIELD_ARRAY);
   TYPE_SET = new type(FIELD_SET);
   TYPE_TYPE = new type(FIELD_ANY);
   TYPE_LIST_INT = new list_type(TYPE_INT);
   TYPE_LIST_FLOAT = new list_type(TYPE_FLOAT);
   TYPE_LIST_NODE = new list_type(TYPE_NODE);
   TYPE_LIST_THREAD = new list_type(TYPE_THREAD);
   TYPE_ARRAY_INT = new array_type(TYPE_INT);
}
   
size_t
field_type_size(field_type type)
{
   switch(type) {
      case FIELD_BOOL:
         return sizeof(bool_val);
      case FIELD_INT:
         return sizeof(int_val);
      case FIELD_FLOAT:
         return sizeof(float_val);
      case FIELD_NODE:
         return sizeof(node_val);
		case FIELD_STRING:
      case FIELD_LIST:
      case FIELD_STRUCT:
      case FIELD_ARRAY:
      case FIELD_THREAD:
      case FIELD_SET:
			return sizeof(ptr_val);
      
      default:
         throw type_error("Unrecognized field type " + to_string(type) + " (field_type_size)");
   }
   
   return 0;
}

string
field_type_string(field_type type)
{
   switch(type) {
      case FIELD_BOOL: return string("bool");
      case FIELD_INT: return string("int");
      case FIELD_FLOAT: return string("float");
      case FIELD_NODE: return string("node");
      case FIELD_THREAD: return string("thread");
		case FIELD_STRING: return string("string");
      case FIELD_LIST: return string("list");
      case FIELD_STRUCT: return string("struct");
      case FIELD_ARRAY: return string("array");
      case FIELD_SET: return string("set");
      default:
         throw type_error("Unrecognized field type " + to_string(type) + " (field_type_string)");
	}
	
   assert(false);
   return string("");
}

string
aggregate_type_string(aggregate_type type)
{
   switch(type) {
      case AGG_FIRST: return string("first");
		case AGG_MIN_FLOAT:
		case AGG_MIN_INT:
         return string("min");
		case AGG_MAX_FLOAT:
		case AGG_MAX_INT:
			return string("max");
	   case AGG_SUM_FLOAT:
		case AGG_SUM_INT:
		case AGG_SUM_LIST_FLOAT:
			return string("sum");
	}
	
   assert(false);
   return string("");
}

}
