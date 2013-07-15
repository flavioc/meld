
#include <cstdlib>
#include <assert.h>

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "utils/utils.hpp"

using namespace vm;
using namespace std;
using namespace utils;

namespace vm {
   
size_t
field_type_size(field_type type)
{
   switch(type) {
      case FIELD_INT:
         return sizeof(int_val);
      case FIELD_FLOAT:
         return sizeof(float_val);
      case FIELD_NODE:
         return sizeof(node_val);
		case FIELD_STRING:
			return sizeof(ptr_val);
      case FIELD_LIST_INT:
      case FIELD_LIST_FLOAT:
      case FIELD_LIST_NODE:
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
      case FIELD_INT: return string("int");
      case FIELD_FLOAT: return string("float");
      case FIELD_NODE: return string("node");
      case FIELD_LIST_INT: return string("int list");
      case FIELD_LIST_FLOAT: return string("float list");
		case FIELD_LIST_NODE: return string("node list");
		case FIELD_STRING: return string("string");
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
