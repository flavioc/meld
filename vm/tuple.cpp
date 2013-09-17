#include <assert.h>
#include <iostream>

#include "vm/tuple.hpp"
#include "db/node.hpp"
#include "utils/utils.hpp"
#include "vm/state.hpp"
#include "utils/serialization.hpp"
#ifdef USE_UI
#include "ui/macros.hpp"
#endif

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
   memset(fields, 0, sizeof(tuple_field) * pred->num_fields());
}

tuple::tuple(void):
   pred(NULL), fields(NULL)
{
}

static bool
equal_int(const tuple_field& f1, const tuple_field& f2)
{
   return f1.int_field == f2.int_field;
}

static bool
equal_float(const tuple_field& f1, const tuple_field& f2)
{
   return f1.float_field == f2.float_field;
}

static bool
equal_node(const tuple_field& f1, const tuple_field& f2)
{
   return f1.node_field == f2.node_field;
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
         if(!cons::equal(get_cons(i), other.get_cons(i), equal_int))
            return false;
         break;
      case FIELD_LIST_FLOAT:
         if(!cons::equal(get_cons(i), other.get_cons(i), equal_float))
            return false;
         break;
      case FIELD_LIST_NODE:
         if(!cons::equal(get_cons(i), other.get_cons(i), equal_node))
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
   if(get_predicate() != other.get_predicate())
      return false;

   for(field_num i = 0; i < num_fields(); ++i)
      if(!field_equal(other, i))
         return false;
   
   return true;
}

void
tuple::copy_field(tuple *ret, const field_num i) const
{
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
         ret->set_cons(i, get_cons(i));
         break;
      case FIELD_LIST_FLOAT:
         ret->set_cons(i, get_cons(i));
         break;
      case FIELD_LIST_NODE:
         ret->set_cons(i, get_cons(i));
         break;
      default:
         throw type_error("Unrecognized field type " + to_string((int)i) + ": " + to_string(get_field_type(i)));
   }
}

tuple*
tuple::copy(void) const
{
   assert(pred != NULL);
   
   tuple *ret(new tuple(get_predicate()));
   
   for(size_t i(0); i < num_fields(); ++i)
      copy_field(ret, i);
   
   return ret;
}

tuple*
tuple::copy_except(const field_num field) const
{
   assert(pred != NULL);
   
   tuple *ret(new tuple(get_predicate()));
   
   for(size_t i(0); i < num_fields(); ++i) {
      if(i != field)
         copy_field(ret, i);
   }
   
   return ret;
}

static inline void
print_int(ostream& out, const tuple_field& val)
{
   out << val.int_field;
}

static inline void
print_float(ostream& out, const tuple_field& val)
{
   out << val.float_field;
}

static inline void
print_node(ostream& out, const tuple_field& val)
{
   out << "@" << val.node_field;
}

void
tuple::print(ostream& cout) const
{
   assert(pred != NULL);

	if(is_persistent())
		cout << "!";
   
   cout << pred_name() << "(";
   
   for(field_num i = 0; i < num_fields(); ++i) {
      if(i != 0)
         cout << ", ";
         
      switch(get_field_type(i)) {
         case FIELD_INT:
            print_int(cout, get_field(i));
            break;
         case FIELD_FLOAT:
            print_float(cout, get_field(i));
            break;
         case FIELD_LIST_INT:
            cons::print(cout, get_cons(i), print_int);
            break;
         case FIELD_LIST_FLOAT:
            cons::print(cout, get_cons(i), print_float);
            break;
         case FIELD_LIST_NODE:
            cons::print(cout, get_cons(i), print_node);
            break;
         case FIELD_NODE:
            print_node(cout, get_field(i));
            break;
			case FIELD_STRING:
				cout << '"' << get_string(i)->get_content() << '"';
				break;
         default:
            throw type_error("Unrecognized field type " + to_string(i) + " (print)");
      }
   }
   
   cout << ")";
}

#ifdef USE_UI
using namespace json_spirit;

Value
tuple::dump_json(void) const
{
	Object ret;

	UI_ADD_FIELD(ret, "predicate", (int)get_predicate_id());
	Array fields;

	for(field_num i = 0; i < num_fields(); ++i) {
		switch(get_field_type(i)) {
			case FIELD_INT:
				UI_ADD_ELEM(fields, get_int(i));
				break;
			case FIELD_FLOAT:
				UI_ADD_ELEM(fields, get_float(i));
				break;
			case FIELD_NODE:
				UI_ADD_ELEM(fields, (int)get_node(i));
				break;
			case FIELD_STRING:
				UI_ADD_ELEM(fields, get_string(i));
				break;
			case FIELD_LIST_INT: {
					Array vec;
					list<int_val> l(int_list::stl_list(get_int_list(i)));

					for(list<int_val>::const_iterator it(l.begin()), end(l.end()); it != end; ++it)
						UI_ADD_ELEM(vec, *it);

					UI_ADD_ELEM(fields, vec);
				}
				break;
			default:
				throw type_error("Unrecognized field type " + to_string(i) + " (tuple::dump_json)");
		}
	}

	UI_ADD_FIELD(ret, "fields", fields);

	return ret;
}
#endif

tuple::~tuple(void)
{
   for(field_num i = 0; i < num_fields(); ++i) {
      switch(get_field_type(i)) {
         case FIELD_LIST_NODE:
         case FIELD_LIST_FLOAT:
         case FIELD_LIST_INT: cons::dec_refs(get_cons(i)); break;
         case FIELD_STRING: get_string(i)->dec_refs(); break;
         default: break;
      }
   }
   
   allocator<tuple_field>().deallocate(fields, pred->num_fields());
}


size_t
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
         case FIELD_LIST_FLOAT:
         case FIELD_LIST_NODE:
            ret += cons::size_list(get_cons(i));
            break;
         default:
            throw type_error("unsupport field type in tuple::get_storage_size");
      }
   }
   
   return ret;
}

void
tuple::pack(byte *buf, const size_t buf_size, int *pos) const
{
   const predicate_id id(get_predicate_id());
   
   assert(*pos <= (int)buf_size);
   utils::pack<predicate_id>((void*)&id, 1, buf, buf_size, pos);
   assert(*pos <= (int)buf_size);
   
   for(field_num i(0); i < num_fields(); ++i) {
      switch(get_field_type(i)) {
         case FIELD_INT: {
               const int_val val(get_int(i));
               utils::pack<int_val>((void*)&val, 1, buf, buf_size, pos);
            }
            break;
         case FIELD_FLOAT: {
               const float_val val(get_float(i));
               utils::pack<float_val>((void*)&val, 1, buf, buf_size, pos);
            }
            break;
         case FIELD_NODE: {
               const node_val val(get_node(i));
               utils::pack<node_val>((void*)&val, 1, buf, buf_size, pos);
            }
            break;
         case FIELD_LIST_INT:
            cons::pack(get_cons(i), buf, buf_size, pos);
            break;
         case FIELD_LIST_FLOAT:
            cons::pack(get_cons(i), buf, buf_size, pos);
            break;
         case FIELD_LIST_NODE:
            cons::pack(get_cons(i), buf, buf_size, pos);
            break;
         default:
            throw type_error("unsupported field type to pack");
      }
      assert(*pos <= (int)buf_size);
   }
}

void
tuple::load(byte *buf, const size_t buf_size, int *pos)
{
   for(field_num i(0); i < num_fields(); ++i) {
      switch(get_field_type(i)) {
         case FIELD_INT: {
               int_val val;
               utils::unpack<int_val>(buf, buf_size, pos, &val, 1);
               set_int(i, val);
            }
            break;
         case FIELD_FLOAT: {
               float_val val;
               utils::unpack<float_val>(buf, buf_size, pos, &val, 1);
               set_float(i, val);
            }
            break;
         case FIELD_NODE: {
               node_val val;
               utils::unpack<node_val>(buf, buf_size, pos, &val, 1);
               set_node(i, val);
            }
            break;
         case FIELD_LIST_INT:
            set_cons(i, cons::unpack(buf, buf_size, pos));
            break;
         case FIELD_LIST_FLOAT:
            set_cons(i, cons::unpack(buf, buf_size, pos));
            break;
         case FIELD_LIST_NODE:
            set_cons(i, cons::unpack(buf, buf_size, pos));
            break;
         default:
            throw type_error("unsupported field type to unpack");
      }
   }
}

tuple*
tuple::unpack(byte *buf, const size_t buf_size, int *pos, vm::program *prog)
{
   predicate_id pred_id;
   
   utils::unpack<predicate_id>(buf, buf_size, pos, &pred_id, 1);
   
   tuple *ret(new tuple(prog->get_predicate(pred_id)));
   
   ret->load(buf, buf_size, pos);
   
   return ret;
   
}

void
tuple::copy_runtime(void)
{
   for(field_num i(0); i < num_fields(); ++i) {
      switch(get_field_type(i)) {
         case FIELD_LIST_FLOAT:
         case FIELD_LIST_NODE:
         case FIELD_LIST_INT: {
               cons *old(get_cons(i));
               cons *ne(cons::copy(old));

               cons::dec_refs(old);
               set_cons(i, ne);
            }
            break;
         case FIELD_STRING: {
               rstring::ptr old(get_string(i));
               rstring::ptr ne(old->copy());

               old->dec_refs();
               set_string(i, ne);
            }
            break;
         default: break;
      }
   }
}

ostream& operator<<(ostream& cout, const tuple& tuple)
{
   tuple.print(cout);
   return cout;
}

}
