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
#include "db/node.hpp"
#include "thread/thread.hpp"

using namespace vm;
using namespace std;
using namespace runtime;
using namespace utils;

namespace vm {

static inline bool value_equal(type *t, const tuple_field &v1,
                               const tuple_field &v2) {
   switch (t->get_type()) {
      case FIELD_INT:
         return FIELD_INT(v1) == FIELD_INT(v2);
      case FIELD_FLOAT:
         return FIELD_FLOAT(v1) == FIELD_FLOAT(v2);
      case FIELD_NODE:
         return FIELD_NODE(v1) == FIELD_NODE(v2);
      case FIELD_THREAD:
         return FIELD_THREAD(v1) == FIELD_THREAD(v2);
      case FIELD_LIST: {
         list_type *lt((list_type *)t);
         type *st(lt->get_subtype());
         runtime::cons *l1(FIELD_CONS(v1));
         runtime::cons *l2(FIELD_CONS(v2));

         if (runtime::cons::is_null(l1) && runtime::cons::is_null(l2))
            return true;

         if (runtime::cons::is_null(l1) && !runtime::cons::is_null(l2))
            return false;

         if (!runtime::cons::is_null(l1) && runtime::cons::is_null(l2))
            return false;

         const tuple_field head1(l1->get_head());
         const tuple_field head2(l2->get_head());
         tuple_field tail1, tail2;
         SET_FIELD_CONS(tail1, l1->get_tail());
         SET_FIELD_CONS(tail2, l2->get_tail());

         if (!value_equal(st, head1, head2)) return false;
         return value_equal(t, tail1, tail2);
      }
      default:
         throw type_error("Unrecognized field type " + t->string());
   }
   return false;
}

bool tuple::field_equal(type *ty, const tuple &other, const field_num i) const {
   return value_equal(ty, get_field(i), other.get_field(i));
}

bool tuple::equal(const tuple &other, predicate *pred) const {
   for (field_num i = 0; i < pred->num_fields(); ++i)
      if (!field_equal(pred->get_field_type(i), other, i)) return false;

   return true;
}

void tuple::copy_field(type *ty, tuple *ret, const field_num i) const {
   switch (ty->get_type()) {
      case FIELD_INT:
         ret->set_int(i, get_int(i));
         break;
      case FIELD_FLOAT:
         ret->set_float(i, get_float(i));
         break;
      case FIELD_NODE:
         ret->set_node(i, get_node(i));
         break;
      case FIELD_THREAD:
         ret->set_thread(i, get_thread(i));
         break;
      case FIELD_LIST:
         ret->set_cons(i, get_cons(i));
         break;
      default:
         throw type_error("Unrecognized field type " + to_string((int)i) +
                          ": " + to_string(ty));
   }
}

tuple *tuple::copy(vm::predicate *pred, mem::node_allocator *alloc) const {
   assert(pred != nullptr);

   tuple *ret(tuple::create(pred, alloc));

   for (size_t i(0); i < pred->num_fields(); ++i)
      copy_field(pred->get_field_type(i), ret, i);

   return ret;
}

tuple *tuple::copy_except(predicate *pred, const field_num field, mem::node_allocator *alloc) const {
   assert(pred != nullptr);

   tuple *ret(tuple::create(pred, alloc));

   for (size_t i(0); i < pred->num_fields(); ++i) {
      if (i != field) copy_field(pred->get_field_type(i), ret, i);
   }

   return ret;
}

static inline void print_int(ostream &out, const tuple_field &val) {
   out << FIELD_INT(val);
}

static inline void print_float(ostream &out, const tuple_field &val) {
   out << std::setprecision(5) << FIELD_FLOAT(val);
}

static inline void print_node(ostream &out, const tuple_field &val) {
   out << "@";
#ifdef USE_REAL_NODES
   out << ((db::node *)FIELD_NODE(val))->get_id();
#else
   out << FIELD_NODE(val);
#endif
}

static inline void print_thread(ostream& out, const tuple_field &val) {
   out << "#T" << ((sched::thread*)FIELD_THREAD(val))->get_id();
}

static inline void print_bool(ostream &out, const tuple_field &val) {
   out << (FIELD_BOOL(val) ? "true" : "false");
}

static inline void print_tuple_type(ostream &cout, const tuple_field &field,
                                    type *t, const bool in_list = false) {
   switch (t->get_type()) {
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
      case FIELD_THREAD:
         print_thread(cout, field);
         break;
      case FIELD_STRING:
         cout << '"' << FIELD_STRING(field)->get_content() << '"';
         break;
      case FIELD_ARRAY: {
         runtime::array *a(FIELD_ARRAY(field));
         array_type *typ((array_type *)t);
         type *at(typ->get_base());
         cout << "#[";

         for (size_t i(0); i < a->get_size(); ++i) {
            if (i > 0) cout << ", ";
            print_tuple_type(cout, a->get_item(i), at);
         }
         cout << "]";
      } break;
      case FIELD_SET: {
         runtime::set *a(FIELD_SET(field));
         set_type *typ((set_type *)t);
         type *at(typ->get_base());
         cout << "{";
         size_t i{0};
         for(const auto p : *a) {
            if(i > 0) cout << ", ";
            vm::tuple_field f;
            f.ptr_field = p;
            print_tuple_type(cout, f, at);
            ++i;
         }
         cout << "}";
      } break;
      case FIELD_STRUCT: {
         runtime::struct1 *s(FIELD_STRUCT(field));
         struct_type *st((struct_type*)t);
         cout << ":(";

         for (size_t i(0); i < st->get_size(); ++i) {
            if (i > 0) cout << ", ";
            print_tuple_type(cout, s->get_data(i), st->get_type(i));
         }

         cout << ")";
      } break;
      case FIELD_LIST: {
         if (!in_list) cout << "[";

         runtime::cons *ls(FIELD_CONS(field));
         if (runtime::cons::is_null(ls)) {
            cout << "]";
            break;
         }

         runtime::cons *tail(ls);
         list_type *lt((list_type *)t);
         size_t count{0};
         while(!runtime::cons::is_null(tail)) {
            tuple_field head(tail->get_head());
            print_tuple_type(cout, head, lt->get_subtype(), false);
            tail = tail->get_tail();
            if (!runtime::cons::is_null(tail)) cout << ",";
            count++;
            if(count > 20) {
               cout << "...";
               break;
            }
         }
         cout << "]";
         break;
      }
      default:
         throw type_error("Unrecognized type " + t->string() + " (print)");
   }
}

string tuple::to_str(const vm::predicate *pred) const {
   stringstream ss;

   print(ss, pred);

   return ss.str();
}

void tuple::print(ostream &cout, const vm::predicate *pred) const {
   assert(pred != nullptr);

   if (pred->is_persistent_pred()) cout << "!";

   cout << pred->get_name() << "(";

   for (field_num i = 0; i < pred->num_fields(); ++i) {
      if (i != 0) cout << ", ";

      print_tuple_type(cout, get_field(i), pred->get_field_type(i));
   }

   cout << ")";
}

void tuple::destructor(const predicate *pred, candidate_gc_nodes &gc_nodes) {
   for (field_num i = 0; i < pred->num_fields(); ++i) {
      switch (pred->get_field_type(i)->get_type()) {
         case FIELD_LIST:
            cons::dec_refs(get_cons(i), (vm::list_type*)pred->get_field_type(i), gc_nodes);
            break;
         case FIELD_STRING:
            get_string(i)->dec_refs();
            break;
         case FIELD_THREAD:
            break;
         case FIELD_STRUCT:
            get_struct(i)
                ->dec_refs((struct_type *)pred->get_field_type(i), gc_nodes);
            break;
         case FIELD_ARRAY:
            get_array(i)->dec_refs(
                ((array_type *)pred->get_field_type(i))->get_base(), gc_nodes);
            break;
         case FIELD_SET:
            get_set(i)->dec_refs(
                  ((set_type *)pred->get_field_type(i))->get_base(), gc_nodes);
            break;
         case FIELD_NODE:
#ifdef GC_NODES
         {
            db::node *n((db::node *)get_node(i));
            if (!All->DATABASE->is_initial_node(n)) {
               assert(n->refs > 0);
               if (n->try_garbage_collect()) gc_nodes.insert((vm::node_val)n);
            }
         }
#else
            (void)gc_nodes;
#endif
         break;
         case FIELD_BOOL:
         case FIELD_INT:
         case FIELD_FLOAT:
            break;
         default:
            throw type_error("unsupported field type in tuple::destructor");
            break;
      }
   }
}

void tuple::set_node(const field_num &field, const node_val &val) {
#ifdef GC_NODES
   db::node *n((db::node *)val);
   if (!All->DATABASE->is_initial_node(n)) n->refs++;
#endif

   SET_FIELD_NODE(getfp()[field], val);
}

void tuple::set_field_ref(const field_num &field, const tuple_field &newval,
                          const predicate *pred, candidate_gc_nodes &gc_nodes) {
   const tuple_field oldval(get_field(field));
   set_field(field, newval);
   const type *typ(pred->get_field_type(field));
   const field_type ftype(typ->get_type());
   if (newval.ptr_field == oldval.ptr_field) return;
   runtime::do_increment_runtime(newval, ftype);
   runtime::do_decrement_runtime(oldval, typ, gc_nodes);
}

size_t tuple::get_storage_size(predicate *pred) const {
   size_t ret(sizeof(predicate_id));

   for (size_t i(0); i < pred->num_fields(); ++i) {
      switch (pred->get_field_type(i)->get_type()) {
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

void tuple::pack(vm::predicate *pred, byte *buf, const size_t buf_size,
                 int *pos) const {
   const predicate_id id(pred->get_id());

   assert(*pos <= (int)buf_size);
   utils::pack<predicate_id>((void *)&id, 1, buf, buf_size, pos);
   assert(*pos <= (int)buf_size);

   for (field_num i(0); i < pred->num_fields(); ++i) {
      switch (pred->get_field_type(i)->get_type()) {
         case FIELD_INT: {
            const int_val val(get_int(i));
            utils::pack<int_val>((void *)&val, 1, buf, buf_size, pos);
         } break;
         case FIELD_FLOAT: {
            const float_val val(get_float(i));
            utils::pack<float_val>((void *)&val, 1, buf, buf_size, pos);
         } break;
         case FIELD_NODE: {
            const node_val val(get_node(i));
            utils::pack<node_val>((void *)&val, 1, buf, buf_size, pos);
         } break;
         case FIELD_LIST:
            cons::pack(get_cons(i), buf, buf_size, pos);
            break;
         default:
            throw type_error("unsupported field type to pack");
      }
      assert(*pos <= (int)buf_size);
   }
}

void tuple::load(vm::predicate *pred, byte *buf, const size_t buf_size,
                 int *pos) {
   for (field_num i(0); i < pred->num_fields(); ++i) {
      switch (pred->get_field_type(i)->get_type()) {
         case FIELD_INT: {
            int_val val;
            utils::unpack<int_val>(buf, buf_size, pos, &val, 1);
            set_int(i, val);
         } break;
         case FIELD_FLOAT: {
            float_val val;
            utils::unpack<float_val>(buf, buf_size, pos, &val, 1);
            set_float(i, val);
         } break;
         case FIELD_NODE: {
            node_val val;
            utils::unpack<node_val>(buf, buf_size, pos, &val, 1);
            set_node(i, val);
         } break;
         case FIELD_LIST:
            set_cons(i, cons::unpack(buf, buf_size, pos,
                                     (list_type *)pred->get_field_type(i)));
            break;
         default:
            throw type_error("unsupported field type to unpack");
      }
   }
}

tuple *tuple::unpack(byte *buf, const size_t buf_size, int *pos,
                     vm::program *prog, mem::node_allocator *alloc) {
   predicate_id pred_id;

   utils::unpack<predicate_id>(buf, buf_size, pos, &pred_id, 1);

   predicate *pred(prog->get_predicate(pred_id));
   tuple *ret(tuple::create(pred, alloc));

   ret->load(pred, buf, buf_size, pos);

   return ret;
}
}
