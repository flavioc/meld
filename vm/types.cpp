
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
      case FIELD_TYPE:
         return sizeof(int_val);
      case FIELD_FLOAT:
         return sizeof(float_val);
      case FIELD_ADDR:
      case FIELD_LIST_INT:
      case FIELD_LIST_FLOAT:
      case FIELD_LIST_ADDR:
         return sizeof(addr_val);
      
      case FIELD_SET_INT:
      case FIELD_SET_FLOAT:
      default:
         throw type_error("Unrecognized field type " + number_to_string(type));
   }
   
   return 0;
}

string
field_type_string(field_type type)
{
   switch(type) {
      case FIELD_INT: return string("int");
      case FIELD_TYPE: return string("type");
      case FIELD_FLOAT: return string("float");
      case FIELD_ADDR: return string("addr");
      case FIELD_LIST_INT: return string("int list");
      case FIELD_LIST_FLOAT: return string("float list");
		case FIELD_LIST_ADDR: return string ("addr list");
		case FIELD_SET_INT: return string("int set");
		case FIELD_SET_FLOAT: return string("float set");
	}
	
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
		case AGG_SUM_LIST_INT:
		case AGG_SUM_LIST_FLOAT:
			return string("sum");
		case AGG_SET_UNION_INT:
		case AGG_SET_UNION_FLOAT:
         return string("set_union");
	}
	
   return string("");
}

}