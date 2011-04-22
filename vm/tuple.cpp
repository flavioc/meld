#include <assert.h>
#include <iostream>

#include "vm/tuple.hpp"
#include "db/node.hpp"
#include "utils/utils.hpp"
#include "vm/state.hpp"

using namespace vm;
using namespace std;
using namespace runtime;
using namespace utils;
using namespace boost;

namespace vm
{
   
tuple::tuple(const predicate* _pred):
   pred((predicate*)_pred), fields(allocator<tuple_field>().allocate(pred->num_fields())) 
{
}

tuple::tuple(void):
   pred(NULL), fields(NULL)
{
}

bool
tuple::field_equal(const tuple& other, const field_num i) const
{
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
   
   return true;
}

bool
tuple::operator==(const tuple& other) const
{
   for(field_num i = 0; i < num_fields(); ++i)
      if(!field_equal(other, i))
         return false;
   
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
         case FIELD_ADDR:
            cout << "@" << get_addr(i);
            break;
         case FIELD_SET_INT:
         case FIELD_SET_FLOAT:
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

#ifdef COMPILE_MPI
void
tuple::save(mpi::packed_oarchive & ar, const unsigned int version) const
{
   ar & get_predicate_id();
   
   for(field_num i(0); i < num_fields(); ++i) {
      switch(get_field_type(i)) {
         case FIELD_INT: {
               int_val val(get_int(i));
               ar & val;
            }
            break;
         case FIELD_FLOAT: {
               float_val val(get_float(i));
               ar & val;
            }
            break;
         case FIELD_ADDR: {
               addr_val val(get_addr(i));
               ar & val;
            }
            break;
         case FIELD_LIST_INT:
            int_list::save_list(ar, get_int_list(i));
            break;
         case FIELD_LIST_FLOAT:
            float_list::save_list(ar, get_float_list(i));
            break;
         case FIELD_LIST_ADDR:
            addr_list::save_list(ar, get_addr_list(i));
            break;
         default:
            throw type_error("unsupported field number " + number_to_string(i));
      }
   }
}
    
void
tuple::load(mpi::packed_iarchive& ar, const unsigned int version)
{
   predicate_id pred_id;
   
   ar & pred_id;
   
   pred = state::PROGRAM->get_predicate(pred_id);
   
   if(fields)
      delete []fields;
      
   fields = new tuple_field[pred->num_fields()];
   
   for(field_num i(0); i < num_fields(); ++i) {
      switch(get_field_type(i)) {
         case FIELD_INT: {
               int_val val;
               ar & val;
               set_int(i, val);
            }
            break;
         case FIELD_FLOAT: {
               float_val val;
               ar & val;
               set_float(i, val);
            }
            break;
         case FIELD_ADDR: {
               addr_val val;
               ar & val;
               set_addr(i, val);
            }
            break;
         case FIELD_LIST_INT:
            set_int_list(i, int_list::load_list(ar));
            break;
         case FIELD_LIST_FLOAT:
            set_float_list(i, float_list::load_list(ar));
            break;
         case FIELD_LIST_ADDR:
            set_addr_list(i, addr_list::load_list(ar));
            break;
         case FIELD_SET_INT:
         case FIELD_SET_FLOAT:
         default:
            throw type_error("unsupported field number " + number_to_string(i));
      }
   }
}
#endif

ostream& operator<<(ostream& cout, const tuple& tuple)
{
   tuple.print(cout);
   return cout;
}

}