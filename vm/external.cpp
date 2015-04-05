
#include <unordered_map>
#include <assert.h>
#include <stdlib.h>

#include "vm/external.hpp"
#include "external/math.hpp"
#include "external/utils.hpp"
#include "external/lists.hpp"
#include "external/structs.hpp"
#include "external/strings.hpp"
#include "external/others.hpp"
#include "external/core.hpp"
#include "external/array.hpp"
#include "external/set.hpp"

using namespace std;

namespace vm {

using namespace external;

typedef unordered_map<external_function_id, external_function *>
    hash_external_type;

static external_function_id external_counter(0);
static external_function_id first_custom(0);
static hash_external_type hash_external;
static bool external_functions_initiated(false);

void external_function::set_arg_type(const size_t arg, type *typ) {
   assert(arg < num_args);
   assert(typ);

   spec[arg] = typ;
}

external_function::external_function(external_function_ptr _ptr,
                                     const size_t _num_args, type *_ret,
                                     string _name)
    : num_args(_num_args),
      name(std::move(_name)),
      ptr(_ptr),
      ret(_ret),
      spec(new type *[_num_args]) {
   assert(ret);
   assert(num_args <= EXTERNAL_ARG_LIMIT);
}

external_function::~external_function(void) { delete[] spec; }

external_function_id register_external_function(external_function *ex) {
   hash_external[external_counter] = ex;
   return external_counter++;
}

external_function *lookup_external_function(const external_function_id id) {
   if (!external_functions_initiated) init_external_functions();

   external_function *ret(hash_external[id]);

   assert(ret != nullptr);

   return ret;
}

external_function *lookup_external_function_by_name(const string &name) {
   if (!external_functions_initiated) init_external_functions();

   for (auto &elem : hash_external) {
      external_function *f(elem.second);
      if (f->get_name() == name) return f;
   }
   return nullptr;
}

static inline external_function *external0(external_function_ptr ptr, type *ret,
                                           const string &name) {
   return new external_function(ptr, 0, ret, name);
}

static inline external_function *external1(external_function_ptr ptr, type *ret,
                                           type *arg1, const string &name) {
   external_function *f(new external_function(ptr, 1, ret, name));

   f->set_arg_type(0, arg1);

   return f;
}

static inline external_function *external2(external_function_ptr ptr, type *ret,
                                           type *arg1, type *arg2,
                                           const string &name) {
   external_function *f(new external_function(ptr, 2, ret, name));

   f->set_arg_type(0, arg1);
   f->set_arg_type(1, arg2);

   return f;
}

static inline external_function *external3(external_function_ptr ptr, type *ret,
                                           type *arg1, type *arg2, type *arg3,
                                           const string &name) {
   external_function *f(new external_function(ptr, 3, ret, name));

   f->set_arg_type(0, arg1);
   f->set_arg_type(1, arg2);
   f->set_arg_type(2, arg3);

   return f;
}

static inline external_function *external4(external_function_ptr ptr, type *ret,
                                           type *arg1, type *arg2, type *arg3,
                                           type *arg4, const string &name) {
   external_function *f(new external_function(ptr, 4, ret, name));

   f->set_arg_type(0, arg1);
   f->set_arg_type(1, arg2);
   f->set_arg_type(2, arg3);
   f->set_arg_type(3, arg4);

   return f;
}

static inline external_function *external5(external_function_ptr ptr, type *ret,
                                           type *arg1, type *arg2, type *arg3,
                                           type *arg4, type *arg5,
                                           const string &name) {
   external_function *f(new external_function(ptr, 5, ret, name));

   f->set_arg_type(0, arg1);
   f->set_arg_type(1, arg2);
   f->set_arg_type(2, arg3);
   f->set_arg_type(3, arg4);
   f->set_arg_type(4, arg5);

   return f;
}

static inline external_function *external6(external_function_ptr ptr, type *ret,
                                           type *arg1, type *arg2, type *arg3,
                                           type *arg4, type *arg5, type *arg6,
                                           const string &name) {
   external_function *f(new external_function(ptr, 6, ret, name));

   f->set_arg_type(0, arg1);
   f->set_arg_type(1, arg2);
   f->set_arg_type(2, arg3);
   f->set_arg_type(3, arg4);
   f->set_arg_type(4, arg5);
   f->set_arg_type(5, arg6);

   return f;
}
static void cleanup_externals(void) {
   for (auto &elem : hash_external) delete elem.second;
}

void register_custom_external_function(external_function_ptr ptr,
                                       const size_t num_args, type *ret,
                                       type **args, const string &name) {
   switch (num_args) {
      case 0:
         register_external_function(external0(ptr, ret, name));
         break;
      case 1:
         register_external_function(external1(ptr, ret, args[0], name));
         break;
      case 2:
         register_external_function(
             external2(ptr, ret, args[0], args[1], name));
         break;
      case 3:
         register_external_function(
             external3(ptr, ret, args[0], args[1], args[2], name));
         break;
      default:
         assert(false);
   }
}

void init_external_functions(void) {
#define EXTERN(NAME) (external_function_ptr) external::NAME
#define EXTERNAL0(NAME, RET) external0(EXTERN(NAME), RET, #NAME)
#define EXTERNAL1(NAME, RET, ARG1) external1(EXTERN(NAME), RET, ARG1, #NAME)
#define EXTERNAL2(NAME, RET, ARG1, ARG2) \
   external2(EXTERN(NAME), RET, ARG1, ARG2, #NAME)
#define EXTERNAL3(NAME, RET, ARG1, ARG2, ARG3) \
   external3(EXTERN(NAME), RET, ARG1, ARG2, ARG3, #NAME)
#define EXTERNAL4(NAME, RET, ARG1, ARG2, ARG3, ARG4) \
   external4(EXTERN(NAME), RET, ARG1, ARG2, ARG3, ARG4, #NAME)
#define EXTERNAL5(NAME, RET, ARG1, ARG2, ARG3, ARG4, ARG5) \
   external5(EXTERN(NAME), RET, ARG1, ARG2, ARG3, ARG4, ARG5, #NAME)
#define EXTERNAL6(NAME, RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) \
   external6(EXTERN(NAME), RET, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, #NAME)
   (void)external5;
   (void)external6;

   if (external_functions_initiated) return;
   external_functions_initiated = true;
   init_types();

   static type *i(TYPE_INT);
   static type *f(TYPE_FLOAT);
   static type *n(TYPE_NODE);
   static type *s(TYPE_STRING);
   static type *b(TYPE_BOOL);
   static type *st(TYPE_STRUCT);
   static type *lt(TYPE_LIST);
   static type *at(TYPE_ARRAY);
   static type *set(TYPE_SET);
   static type *any(TYPE_ANY);
   static type *t(TYPE_TYPE);
   static list_type *li(TYPE_LIST_INT);
   static list_type *lf(TYPE_LIST_FLOAT);
   static list_type *ln(TYPE_LIST_NODE);
   static array_type *ai(TYPE_ARRAY_INT);

   register_external_function(EXTERNAL1(sigmoid, f, f));
   register_external_function(EXTERNAL1(randint, i, i));
   register_external_function(EXTERNAL1(normalize, lf, lf));
   register_external_function(EXTERNAL3(damp, lf, lf, lf, f));
   register_external_function(EXTERNAL2(divide, lf, lf, lf));
   register_external_function(EXTERNAL2(convolve, lf, lf, lf));
   register_external_function(EXTERNAL2(addfloatlists, lf, lf, lf));
   register_external_function(EXTERNAL2(intlistdiff, li, li, li));
   register_external_function(EXTERNAL2(lnth, any, lt, i));
   register_external_function(EXTERNAL2(concatenate, s, s, s));
   register_external_function(EXTERNAL1(str2float, f, s));
   register_external_function(EXTERNAL1(str2int, i, s));
   register_external_function(EXTERNAL3(lremove, ln, ln, n, i));
   register_external_function(EXTERNAL1(wastetime, i, i));
   register_external_function(EXTERNAL2(truncate, f, f, i));
   register_external_function(EXTERNAL1(float2int, i, f));
   register_external_function(EXTERNAL1(int2str, s, i));
   register_external_function(EXTERNAL1(float2str, s, f));
   register_external_function(EXTERNAL3(intlistsub, li, li, i, i));
   register_external_function(EXTERNAL1(str2intlist, li, s));
   register_external_function(EXTERNAL2(filecountwords, i, s, i));
   register_external_function(EXTERNAL2(residual, f, lf, lf));
   register_external_function(EXTERNAL1(llength, i, lt));
   register_external_function(EXTERNAL3(lappend, lt, lt, lt, t));
   register_external_function(EXTERNAL1(priority, f, n));
   register_external_function(EXTERNAL2(lreverse, lt, lt, i));
   register_external_function(EXTERNAL1(listlast, n, ln));
   register_external_function(EXTERNAL1(cpu_id, i, n));
   register_external_function(EXTERNAL1(node2int, i, n));
   register_external_function(EXTERNAL2(intpower, i, i, i));
   register_external_function(EXTERNAL2(lsort, li, li, i));
   register_external_function(EXTERNAL2(lremoveduplicates, li, li, i));
   register_external_function(EXTERNAL2(degeneratevector, li, i, i));
   register_external_function(EXTERNAL2(demergemessages, li, li, li));
   register_external_function(EXTERNAL2(intlistequal, i, i, i));
   register_external_function(EXTERNAL3(addfloatstructs, st, st, st, t));
   register_external_function(EXTERNAL2(normalizestruct, st, st, i));
   register_external_function(EXTERNAL4(dampstruct, st, st, st, f, i));
   register_external_function(EXTERNAL3(residualstruct, f, st, st, i));
   register_external_function(EXTERNAL3(dividestruct, st, st, st, i));
   register_external_function(EXTERNAL3(convolvestruct, st, st, st, i));
   register_external_function(EXTERNAL1(cpu_static, i, n));
   register_external_function(EXTERNAL1(is_moving, b, n));
   register_external_function(EXTERNAL1(is_static, b, n));
   register_external_function(EXTERNAL4(partition_vertical, i, i, i, i, i));
   register_external_function(EXTERNAL4(partition_horizontal, i, i, i, i, i));
   register_external_function(EXTERNAL4(partition_grid, i, i, i, i, i));
   register_external_function(EXTERNAL1(queue_size, i, n));
   register_external_function(EXTERNAL1(facts_proved, i, n));
   register_external_function(EXTERNAL1(facts_consumed, i, n));
   register_external_function(EXTERNAL3(listcount, i, lt, i, i));
   register_external_function(EXTERNAL3(lexists, b, lt, i, t));
   register_external_function(EXTERNAL3(lexistss, b, lt, lt, t));
   register_external_function(EXTERNAL2(queens_violation, b, i, li));
   register_external_function(EXTERNAL3(minimax_score, i, li, i, i));
   register_external_function(EXTERNAL2(minimax_points, i, li, i));
   register_external_function(EXTERNAL3(array_init, at, i, any, i));
   register_external_function(EXTERNAL2(array_get, any, at, i));
   register_external_function(EXTERNAL4(array_set, at, at, i, any, i));
   register_external_function(EXTERNAL3(array_add, at, at, any, i));
   register_external_function(EXTERNAL2(array_from_list, at, lt, i));
   register_external_function(EXTERNAL1(array_size, i, at));
   register_external_function(EXTERNAL3(minimax_score2, i, ai, i, i));
   register_external_function(EXTERNAL2(minimax_points2, i, ai, i));
   register_external_function(EXTERNAL1(myfabs, f, f));
   register_external_function(EXTERNAL1(set_size, i, set));
   register_external_function(EXTERNAL3(set_exists, b, set, any, t));
   register_external_function(EXTERNAL3(set_add, set, set, any, t));
   register_external_function(EXTERNAL2(set_from_list, set, lt, t));
   register_external_function(EXTERNAL3(array_exists, b, at, any, t));
   register_external_function(EXTERNAL2(ltake, lt, lt, i));

   atexit(cleanup_externals);

   first_custom = external_counter;
}

external_function_id first_custom_external_function(void) {
   return first_custom;
}
}
