#include <assert.h>
#include <iostream>
#include <sstream>
#include <limits>
#include <iomanip>

#include "vm/tuple.hpp"
#include "db/database.hpp"
#include "db/node.hpp"
#include "utils/utils.hpp"
#include "vm/state.hpp"
#include "utils/serialization.hpp"
#ifdef USE_UI
#include "ui/macros.hpp"
#endif
#include "db/node.hpp"

using namespace vm;
using namespace std;
using namespace runtime;
using namespace utils;

namespace vm
{
   
static inline bool
value_equal(type *t, const tuple_field& v1, const tuple_field& v2)
{
   switch(t->get_type()) {
      case FIELD_INT: return FIELD_INT(v1) == FIELD_INT(v2);
      case FIELD_FLOAT: return FIELD_FLOAT(v1) == FIELD_FLOAT(v2);
      case FIELD_NODE: return FIELD_NODE(v1) == FIELD_NODE(v2);
      case FIELD_LIST: {
         list_type *lt((list_type*)t);
         type *st(lt->get_subtype());
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

         if(!value_equal(st, head1, head2))
            return false;
         return value_equal(t, tail1, tail2);
      }
      break;
      default:
         throw type_error("Unrecognized field type " + t->string());
   }
   return false;
}

bool
tuple::field_equal(type *ty, const tuple& other, const field_num i) const
{
   return value_equal(ty, get_field(i), other.get_field(i));
}

bool
tuple::equal(const tuple& other, predicate *pred) const
{
   for(field_num i = 0; i < pred->num_fields(); ++i)
      if(!field_equal(pred->get_field_type(i), other, i))
         return false;
   
   return true;
}

void
tuple::copy_field(type *ty, tuple *ret, const field_num i) const
{
   switch(ty->get_type()) {
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
         throw type_error("Unrecognized field type " + to_string((int)i) + ": " + to_string(ty));
   }
}

tuple*
tuple::copy(vm::predicate *pred) const
{
   assert(pred != NULL);
   
   tuple *ret(tuple::create(pred));
   
   for(size_t i(0); i < pred->num_fields(); ++i)
      copy_field(pred->get_field_type(i), ret, i);
   
   return ret;
}

tuple*
tuple::copy_except(predicate *pred, const field_num field) const
{
   assert(pred != NULL);
   
   tuple *ret(tuple::create(pred));
   
   for(size_t i(0); i < pred->num_fields(); ++i) {
      if(i != field)
         copy_field(pred->get_field_type(i), ret, i);
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
   out << std::setprecision(5) << FIELD_FLOAT(val);
}

static inline void
print_node(ostream& out, const tuple_field& val)
{
   out << "@";
#ifdef USE_REAL_NODES
   out << ((db::node*)FIELD_NODE(val))->get_id();
#else
   out << FIELD_NODE(val);
#endif
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

         tuple_field head(ls->get_head());
         runtime::cons *tail(ls->get_tail());
         list_type *lt((list_type*)t);
         tuple_field arg;
         SET_FIELD_CONS(arg, tail);
         print_tuple_type(cout, head, lt->get_subtype(), false);
         if(!runtime::cons::is_null(tail))
            cout << ",";
         print_tuple_type(cout, arg, t, true);
         break;
      }
      default:
            throw type_error("Unrecognized type " + t->string() + " (print)");
   }
}

string
tuple::to_str(const vm::predicate *pred) const
{
   stringstream ss;

   print(ss, pred);

   return ss.str();
}

void
tuple::print(ostream& cout, const vm::predicate *pred) const
{
   assert(pred != NULL);

	if(pred->is_persistent_pred())
		cout << "!";
   
   cout << pred->get_name() << "(";
   
   for(field_num i = 0; i < pred->num_fields(); ++i) {
      if(i != 0)
         cout << ", ";

      print_tuple_type(cout, get_field(i), pred->get_field_type(i));
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
tuple::dump_json(const vm::predicate *pred) const
{
	Object ret;

	UI_ADD_FIELD(ret, "predicate", (int)get_predicate_id());
	Array fields;

	for(field_num i = 0; i < pred->num_fields(); ++i) {
      UI_ADD_ELEM(fields, value_to_json_value(pred->get_field_type(i), get_field(i)));
	}

	UI_ADD_FIELD(ret, "fields", fields);

	return ret;
}
#endif

void
tuple::destructor(predicate *pred
#ifdef GC_NODES
      , candidate_gc_nodes& gc_nodes
#endif
      )
{
   for(field_num i = 0; i < pred->num_fields(); ++i) {
      switch(pred->get_field_type(i)->get_type()) {
         case FIELD_LIST: cons::dec_refs(get_cons(i)); break;
         case FIELD_STRING: get_string(i)->dec_refs(); break;
         case FIELD_STRUCT: get_struct(i)->dec_refs(); break;
         case FIELD_NODE:
#ifdef GC_NODES
            {
               db::node *n((db::node*)get_node(i));
               if(!All->DATABASE->is_initial_node(n)) {
                  n->refs--;
                  if(n->garbage_collect())
                     gc_nodes.insert(get_node(i));
               }
            }
#endif
            break;
         case FIELD_BOOL:
         case FIELD_INT:
         case FIELD_FLOAT:
            break;
         default: assert(false); break;
      }
   }
}

void
tuple::set_node(const field_num& field, const node_val& val)
{
#ifdef GC_NODES
   db::node *n((db::node*)val);
   if(!All->DATABASE->is_initial_node(n))
      n->refs++;
#endif

   SET_FIELD_NODE(getfp()[field], val);
}

size_t
tuple::get_storage_size(predicate *pred) const
{
   size_t ret(sizeof(predicate_id));

   for(size_t i(0); i < pred->num_fields(); ++i) {
      switch(pred->get_field_type(i)->get_type()) {
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
tuple::pack(vm::predicate *pred, byte *buf, const size_t buf_size, int *pos) const
{
   const predicate_id id(pred->get_id());
   
   assert(*pos <= (int)buf_size);
   utils::pack<predicate_id>((void*)&id, 1, buf, buf_size, pos);
   assert(*pos <= (int)buf_size);
   
   for(field_num i(0); i < pred->num_fields(); ++i) {
      switch(pred->get_field_type(i)->get_type()) {
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
tuple::load(vm::predicate *pred, byte *buf, const size_t buf_size, int *pos)
{
   for(field_num i(0); i < pred->num_fields(); ++i) {
      switch(pred->get_field_type(i)->get_type()) {
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
            set_cons(i, cons::unpack(buf, buf_size, pos, (list_type*)pred->get_field_type(i)));
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
   
   predicate *pred(prog->get_predicate(pred_id));
   tuple *ret(tuple::create(pred));
   
   ret->load(pred, buf, buf_size, pos);
   
   return ret;
   
}

}
