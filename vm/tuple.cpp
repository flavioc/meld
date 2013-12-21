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
   to_delete(false), pred((predicate*)_pred),
   fields(allocator<tuple_field>().allocate(pred->num_fields()))
{
   assert(pred != NULL);
   memset(fields, 0, sizeof(tuple_field) * pred->num_fields());
}

static inline bool
value_equal(type *t1, type *t2, const tuple_field& v1, const tuple_field& v2)
{
   if(!t1->equal(t2))
      return false;

   switch(t1->get_type()) {
      case FIELD_INT: return FIELD_INT(v1) == FIELD_INT(v2);
      case FIELD_FLOAT: return FIELD_FLOAT(v1) == FIELD_FLOAT(v2);
      case FIELD_NODE: return FIELD_NODE(v1) == FIELD_NODE(v2);
      case FIELD_LIST: {
         list_type *lt1((list_type*)t1);
         list_type *lt2((list_type*)t2);
         type *st1(lt1->get_subtype());
         type *st2(lt2->get_subtype());
         runtime::cons *l1(FIELD_CONS(v1));
         runtime::cons *l2(FIELD_CONS(v2));

         if(runtime::cons::is_null(l1) && runtime::cons::is_null(l2))
            return true;

         if(runtime::cons::is_null(l1) && !runtime::cons::is_null(l2))
            return false;

         if(!runtime::cons::is_null(l1) && runtime::cons::is_null(l2))
            return false;

         const tuple_field head1(l1->get_head());
         const tuple_field head2(l2->get_head());
         tuple_field tail1, tail2;
         SET_FIELD_CONS(tail1, l1->get_tail());
         SET_FIELD_CONS(tail2, l2->get_tail());

         if(!value_equal(st1, st2, head1, head2))
            return false;
         return value_equal(t1, t2, tail1, tail2);
      }
      break;
      default:
         throw type_error("Unrecognized field type " + t1->string());
   }
   return false;
}

bool
tuple::field_equal(const tuple& other, const field_num i) const
{
   assert(i < num_fields());
   
   return value_equal(get_field_type(i), other.get_field_type(i), get_field(i), other.get_field(i));
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
   switch(get_field_type(i)->get_type()) {
      case FIELD_INT:
         ret->set_int(i, get_int(i));
         break;
      case FIELD_FLOAT:
         ret->set_float(i, get_float(i));
         break;
      case FIELD_NODE:
         ret->set_node(i, get_node(i));
         break;
      case FIELD_LIST:
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
   out << FIELD_INT(val);
}

static inline void
print_float(ostream& out, const tuple_field& val)
{
   out << FIELD_FLOAT(val);
}

static inline void
print_node(ostream& out, const tuple_field& val)
{
   out << "@" << FIELD_NODE(val);
}

static inline void
print_bool(ostream& out, const tuple_field& val)
{
   out << (FIELD_BOOL(val) ? "true" : "false");
}

static inline void
print_tuple_type(ostream& cout, const tuple_field& field, type *t, const bool in_list = false)
{
   switch(t->get_type()) {
      case FIELD_BOOL:
         print_bool(cout, field);
         break;
      case FIELD_INT:
         print_int(cout, field);
         break;
      case FIELD_FLOAT:
         print_float(cout, field);
         break;
      case FIELD_NODE:
         print_node(cout, field);
         break;
      case FIELD_STRING:
         cout << '"' << FIELD_STRING(field)->get_content() << '"';
         break;
      case FIELD_STRUCT: {
         runtime::struct1 *s(FIELD_STRUCT(field));
         struct_type *st(s->get_type());
         cout << ":(";

         for(size_t i(0); i < st->get_size(); ++i) {
            if(i > 0)
               cout << ", ";
            print_tuple_type(cout, s->get_data(i), st->get_type(i));
         }

         cout << ")";
      }
      break;
      case FIELD_LIST: {
         if(!in_list)
            cout << "[";

         runtime::cons *ls(FIELD_CONS(field));
         if(runtime::cons::is_null(ls)) {
            cout << "]";
            break;
         }

         tuple_field head(FIELD_CONS(field)->get_head());
         runtime::cons *tail(FIELD_CONS(field)->get_tail());
         list_type *lt((list_type*)t);
         tuple_field arg;
         SET_FIELD_CONS(arg, tail);
         print_tuple_type(cout, head, lt->get_subtype());
         if(!runtime::cons::is_null(tail))
            cout << ",";
         print_tuple_type(cout, arg, t, true);
         break;
      }
      default:
            throw type_error("Unrecognized type " + t->string() + " (print)");
   }
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

      print_tuple_type(cout, get_field(i), get_field_type(i));
   }
   
   cout << ")";
}

#ifdef USE_UI
using namespace json_spirit;

Value
value_to_json_value(type *t, const tuple_field& val)
{
   switch(t->get_type()) {
      case FIELD_INT: return Value(FIELD_INT(val));
      case FIELD_FLOAT: return Value(FIELD_FLOAT(val));
      case FIELD_NODE: return Value((int_val)FIELD_NODE(val));
      case FIELD_STRING: return Value(FIELD_STRING(val)->get_content());
      case FIELD_LIST: {
         runtime::cons *c(FIELD_CONS(val));
         if(runtime::cons::is_null(c))
            return Array();
         list_type *lt((list_type*)t);
         type *sub(lt->get_subtype());
         tuple_field tail;
         SET_FIELD_CONS(tail, c->get_tail());
         Value v1(value_to_json_value(t, tail));
         Value v2(value_to_json_value(sub, c->get_head()));
         assert(v1.type() == array_type);
         Array& arr(v2.get_array());
         Array new_arr;
         new_arr.push_back(v2);
         for(size_t i(0); i < arr.size(); ++i)
            new_arr.push_back(arr[i]);
         return Value(new_arr);
      }
      break;
      default:
				throw type_error("Unrecognized field type " + t->string());
   }
}

Value
tuple::dump_json(void) const
{
	Object ret;

	UI_ADD_FIELD(ret, "predicate", (int)get_predicate_id());
	Array fields;

	for(field_num i = 0; i < num_fields(); ++i) {
      UI_ADD_ELEM(fields, value_to_json_value(get_field_type(i), get_field(i)));
	}

	UI_ADD_FIELD(ret, "fields", fields);

	return ret;
}
#endif

tuple::~tuple(void)
{
   for(field_num i = 0; i < num_fields(); ++i) {
      switch(get_field_type(i)->get_type()) {
         case FIELD_LIST: cons::dec_refs(get_cons(i)); break;
         case FIELD_STRING: get_string(i)->dec_refs(); break;
         case FIELD_STRUCT: get_struct(i)->dec_refs(); break;
         case FIELD_BOOL:
         case FIELD_INT:
         case FIELD_FLOAT:
         case FIELD_NODE: break;
         default: assert(false); break;
      }
   }
   
   allocator<tuple_field>().deallocate(fields, pred->num_fields());
}


size_t
tuple::get_storage_size(void) const
{
   size_t ret(sizeof(predicate_id));

   for(size_t i(0); i < num_fields(); ++i) {
      switch(get_field_type(i)->get_type()) {
         case FIELD_INT:
         case FIELD_FLOAT:
         case FIELD_NODE:
            ret += pred->get_field_size(i);
            break;
         case FIELD_LIST:
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
      switch(get_field_type(i)->get_type()) {
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
         case FIELD_LIST:
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
      switch(get_field_type(i)->get_type()) {
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
         case FIELD_LIST:
            set_cons(i, cons::unpack(buf, buf_size, pos, (list_type*)get_field_type(i)));
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
      switch(get_field_type(i)->get_type()) {
         case FIELD_LIST: {
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
