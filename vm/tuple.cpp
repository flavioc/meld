#include <assert.h>
#include <iostream>

#include "vm/tuple.hpp"
#include "db/node.hpp"
#include "utils/utils.hpp"

using namespace vm;
using namespace std;
using namespace runtime;
using namespace utils;

namespace vm
{
   
tuple::tuple(const predicate* _pred):
   pred(_pred), fields(new tuple_field[pred->num_fields()])
{
}

bool
tuple::operator==(const tuple& other) const
{
   for(field_num i = 0; i < num_fields(); ++i) {
      switch(get_field_type(i)) {
         case FIELD_INT:
            if(get_int(i) != other.get_int(i))
               return false;
            break;
         case FIELD_FLOAT:
            if(get_float(i) != other.get_float(i))
               return false;
            break;
         case FIELD_LIST_INT:
            if(!int_list::equal(get_int_list(i), other.get_int_list(i)))
               return false;
            break;
         case FIELD_LIST_FLOAT:
            if(!float_list::equal(get_float_list(i), other.get_float_list(i)))
               return false;
            break;
         case FIELD_LIST_ADDR:
            if(!addr_list::equal(get_addr_list(i), other.get_addr_list(i)))
               return false;
            break;
         case FIELD_ADDR:
            if(get_addr(i) != other.get_addr(i))
               return false;
            break;
         default:
            throw type_error("Unrecognized field type " + number_to_string(i));
      }
   }
   
   return true;
}

void
tuple::print(ostream& cout) const
{
   cout << pred_name() << "(";
   
   for(field_num i = 0; i < num_fields(); ++i) {
      if(i != 0)
         cout << ", ";
         
      switch(get_field_type(i)) {
         case FIELD_INT:
            cout << get_int(i);
            break;
         case FIELD_FLOAT:
            cout << get_float(i);
            break;
         case FIELD_LIST_INT:
            int_list::print(cout, get_int_list(i));
            break;
         case FIELD_LIST_FLOAT:
            float_list::print(cout, get_float_list(i));
            break;
         case FIELD_LIST_ADDR:
            addr_list::print(cout, get_addr_list(i));
            break;
         case FIELD_ADDR: {
               db::node* node((db::node*)get_addr(i));
               cout << "@" << node->get_id();
               break;
            }
         case FIELD_SET_INT:
         case FIELD_SET_FLOAT:
            cout << get_addr(i);
            break;
         default:
            throw type_error("Unrecognized field type " + number_to_string(i));
      }
   }
   
   cout << ")";
}

tuple::~tuple(void)
{
   for(field_num i = 0; i < num_fields(); ++i) {
      switch(get_field_type(i)) {
         case FIELD_LIST_INT: int_list::dec_refs(get_int_list(i)); break;
         case FIELD_LIST_FLOAT: float_list::dec_refs(get_float_list(i)); break;
         case FIELD_LIST_ADDR: addr_list::dec_refs(get_addr_list(i)); break;
         default: break;
      }
   }
   
   delete []fields;
}

ostream& operator<<(ostream& cout, const tuple& tuple)
{
   tuple.print(cout);
   return cout;
}

}