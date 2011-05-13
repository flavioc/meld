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
   assert(pred != NULL);
}

tuple::tuple(void):
   pred(NULL), fields(NULL)
{
}

bool
tuple::field_equal(const tuple& other, const field_num i) const
{
   assert(i < num_fields());
   
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
      case FIELD_LIST_NODE:
         if(!node_list::equal(get_node_list(i), other.get_node_list(i)))
            return false;
         break;
      case FIELD_NODE:
         if(get_node(i) != other.get_node(i))
            return false;
         break;
      default:
         throw type_error("Unrecognized field type " + to_string((int)i) + ": " + to_string(get_field_type(i)));
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

tuple*
tuple::copy(void) const
{
   assert(pred != NULL);
   
   tuple *ret(new tuple(get_predicate()));
   
   for(size_t i(0); i < num_fields(); ++i) {
      switch(get_field_type(i)) {
         case FIELD_INT:
            ret->set_int(i, get_int(i));
            break;
         case FIELD_FLOAT:
            ret->set_float(i, get_float(i));
            break;
         case FIELD_NODE:
            ret->set_node(i, get_node(i));
            break;
         case FIELD_LIST_INT:
            ret->set_int_list(i, get_int_list(i));
            break;
         case FIELD_LIST_FLOAT:
            ret->set_float_list(i, get_float_list(i));
            break;
         case FIELD_LIST_NODE:
            ret->set_node_list(i, get_node_list(i));
            break;
      }
   }
   
   return ret;
}

void
tuple::print(ostream& cout) const
{
   assert(pred != NULL);
   
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
         case FIELD_LIST_NODE:
            node_list::print(cout, get_node_list(i));
            break;
         case FIELD_NODE:
            cout << "@" << get_node(i);
            break;
         default:
            throw type_error("Unrecognized field type " + to_string(i));
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
         case FIELD_LIST_NODE: node_list::dec_refs(get_node_list(i)); break;
         default: break;
      }
   }
   
   allocator<tuple_field>().deallocate(fields, pred->num_fields());
}

#ifdef COMPILE_MPI
void
tuple::save(mpi::packed_oarchive & ar, const unsigned int version) const
{
   const predicate_id id(get_predicate_id());
   ar & id;
   
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
         case FIELD_NODE: {
               node_val val(get_node(i));
               ar & val;
            }
            break;
         case FIELD_LIST_INT:
            int_list::save_list(ar, get_int_list(i));
            break;
         case FIELD_LIST_FLOAT:
            float_list::save_list(ar, get_float_list(i));
            break;
         case FIELD_LIST_NODE:
            node_list::save_list(ar, get_node_list(i));
            break;
         default:
            throw type_error("unsupported field number " + to_string(i));
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
         case FIELD_NODE: {
               node_val val;
               ar & val;
               set_node(i, val);
            }
            break;
         case FIELD_LIST_INT:
            set_int_list(i, int_list::load_list(ar));
            break;
         case FIELD_LIST_FLOAT:
            set_float_list(i, float_list::load_list(ar));
            break;
         case FIELD_LIST_NODE:
            set_node_list(i, node_list::load_list(ar));
            break;
         default:
            throw type_error("unsupported field number " + to_string(i));
      }
   }
}

const size_t
tuple::get_storage_size(void) const
{
   size_t ret(sizeof(predicate_id));

   for(size_t i(0); i < num_fields(); ++i) {
      switch(get_field_type(i)) {
         case FIELD_INT:
         case FIELD_FLOAT:
         case FIELD_NODE:
            ret += pred->get_field_size(i);
            break;
         case FIELD_LIST_INT:
            ret += int_list::size_list(get_int_list(i), field_type_size(FIELD_INT));
            break;
         case FIELD_LIST_FLOAT:
            ret += float_list::size_list(get_float_list(i), field_type_size(FIELD_FLOAT));
            break;
         case FIELD_LIST_NODE:
            ret += node_list::size_list(get_node_list(i), field_type_size(FIELD_NODE));
            break;
         default:
            throw type_error("unsupport field type in tuple::get_storage_size");
      }
   }
   
   return ret;
}

void
tuple::pack(byte *buf, const size_t buf_size, int *pos, MPI_Comm comm) const
{
   const predicate_id id(get_predicate_id());
   
   assert(*pos <= buf_size);
   MPI_Pack((void*)&id, 1, MPI_UNSIGNED_CHAR, buf, buf_size, pos, comm);
   assert(*pos <= buf_size);
   
   for(field_num i(0); i < num_fields(); ++i) {
      switch(get_field_type(i)) {
         case FIELD_INT: {
               const int_val val(get_int(i));
               MPI_Pack((void*)&val, 1, MPI_INT, buf, buf_size, pos, comm);
            }
            break;
         case FIELD_FLOAT: {
               const float_val val(get_float(i));
               MPI_Pack((void*)&val, 1, MPI_FLOAT, buf, buf_size, pos, comm);
            }
            break;
         case FIELD_NODE: {
               const node_val val(get_node(i));
               MPI_Pack((void*)&val, 1, MPI_UNSIGNED, buf, buf_size, pos, comm);
            }
            break;
         case FIELD_LIST_INT:
            int_list::pack(get_int_list(i), MPI_INT, buf, buf_size, pos, comm);
            break;
         case FIELD_LIST_FLOAT:
            float_list::pack(get_float_list(i), MPI_FLOAT, buf, buf_size, pos, comm);
            break;
         case FIELD_LIST_NODE:
            node_list::pack(get_node_list(i), MPI_UNSIGNED, buf, buf_size, pos, comm);
            break;
         default:
            throw type_error("unsupported field type to pack");
      }
      assert(*pos <= buf_size);
   }
}

void
tuple::load(byte *buf, const size_t buf_size, int *pos, MPI_Comm comm)
{
   for(field_num i(0); i < num_fields(); ++i) {
      switch(get_field_type(i)) {
         case FIELD_INT: {
               int_val val;
               MPI_Unpack(buf, buf_size, pos, &val, 1, MPI_INT, comm);
               set_int(i, val);
            }
            break;
         case FIELD_FLOAT: {
               float_val val;
               MPI_Unpack(buf, buf_size, pos, &val, 1, MPI_FLOAT, comm);
               set_float(i, val);
            }
            break;
         case FIELD_NODE: {
               node_val val;
               MPI_Unpack(buf, buf_size, pos, &val, 1, MPI_UNSIGNED, comm);
               set_node(i, val);
            }
            break;
         case FIELD_LIST_INT:
            set_int_list(i, int_list::unpack(MPI_INT, buf, buf_size, pos, comm));
            break;
         case FIELD_LIST_FLOAT:
            set_float_list(i, float_list::unpack(MPI_FLOAT, buf, buf_size, pos, comm));
            break;
         case FIELD_LIST_NODE:
            set_node_list(i, node_list::unpack(MPI_UNSIGNED, buf, buf_size, pos, comm));
            break;
         default:
            throw type_error("unsupported field type to unpack");
      }
   }
}

tuple*
tuple::unpack(byte *buf, const size_t buf_size, int *pos, MPI_Comm comm)
{
   predicate_id pred_id;
   
   MPI_Unpack(buf, buf_size, pos, &pred_id, 1, MPI_UNSIGNED_CHAR, comm);
   
   tuple *ret(new tuple(state::PROGRAM->get_predicate(pred_id)));
   
   ret->load(buf, buf_size, pos, comm);
   
   return ret;
   
}
#endif

ostream& operator<<(ostream& cout, const tuple& tuple)
{
   tuple.print(cout);
   return cout;
}

}
