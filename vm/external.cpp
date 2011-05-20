
#include <tr1/unordered_map>
#include <assert.h>

#include "vm/external.hpp"
#include "external/math.hpp"

using namespace std;
using namespace std::tr1;

namespace vm
{
   
using namespace external;

static bool init_external_functions(void);
static external_function_id external_counter(0);   
static unordered_map<external_function_id, external_function*> hash_external;
static bool dummy(init_external_functions());

void
external_function::set_arg_type(const size_t arg, const field_type typ)
{
   assert(arg < num_args);
   assert(arg >= 0);
   
   spec[arg] = typ;
}

external_function::external_function(external_function_ptr _ptr,
         const size_t _num_args,
         const field_type _ret):
   num_args(_num_args),
   ptr(_ptr), ret(_ret),
   spec(new field_type[_num_args])
{
   assert(num_args >= 0);
}

external_function::~external_function(void)
{
   delete []spec;
}

external_function_id
register_external_function(external_function *ex)
{
   hash_external[external_counter] = ex;
   return external_counter++;
}

external_function*
lookup_external_function(const external_function_id id)
{
   external_function *ret(hash_external[id]);
   
   assert(ret != NULL);
   
   return ret;
}

static inline external_function*
external0(external_function_ptr ptr, const field_type ret)
{
   return new external_function(ptr, 0, ret);
}

static inline external_function*
external1(external_function_ptr ptr, const field_type ret, const field_type arg1)
{
   external_function *f(new external_function(ptr, 1, ret));
   
   f->set_arg_type(0, arg1);
   
   return f;
}

static inline external_function*
external2(external_function_ptr ptr, const field_type ret, const field_type arg1, const field_type arg2)
{
   external_function *f(new external_function(ptr, 2, ret));
   
   f->set_arg_type(0, arg1);
   f->set_arg_type(1, arg2);
   
   return f;
}

static bool
init_external_functions(void)
{
#define EXTERN(NAME) (external_function_ptr) external :: NAME
#define EXTERNAL0(NAME, RET) external0(EXTERN(NAME), RET)
#define EXTERNAL1(NAME, RET, ARG1) external1(EXTERN(NAME), RET, ARG1)
#define EXTERNAL2(NAME, RET, ARG1, ARG2) external1(EXTERN(NAME), RET, ARG1, ARG2)

   register_external_function(EXTERNAL1(sigmoid, FIELD_FLOAT, FIELD_FLOAT));
   
   return true;
}

}