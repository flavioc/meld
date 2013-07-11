
#include <tr1/unordered_map>
#include <assert.h>
#include <stdlib.h>

#include "vm/external.hpp"
#include "external/math.hpp"
#include "external/utils.hpp"
#include "external/lists.hpp"
#include "external/strings.hpp"
#include "external/others.hpp"
#include "external/core.hpp"

using namespace std;
using namespace std::tr1;

namespace vm
{
   
using namespace external;

typedef unordered_map<external_function_id, external_function*> hash_external_type;

static bool init_external_functions(void);
static external_function_id external_counter(0);   
static hash_external_type hash_external;
static bool dummy(init_external_functions());

void
external_function::set_arg_type(const size_t arg, const field_type typ)
{
   assert(arg < num_args);
   
   spec[arg] = typ;
}

external_function::external_function(external_function_ptr _ptr,
         const size_t _num_args,
         const field_type _ret):
   num_args(_num_args),
   ptr(_ptr), ret(_ret),
   spec(new field_type[_num_args])
{
   assert(num_args <= EXTERNAL_ARG_LIMIT);
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

static inline external_function*
external3(external_function_ptr ptr, const field_type ret, const field_type arg1,
   const field_type arg2, const field_type arg3)
{
   external_function *f(new external_function(ptr, 3, ret));
   
   f->set_arg_type(0, arg1);
   f->set_arg_type(1, arg2);
   f->set_arg_type(2, arg3);
   
   return f;
}

static void
cleanup_externals(void)
{
   for(hash_external_type::iterator it(hash_external.begin()), end(hash_external.end()); it != end; it++)
      delete it->second;
}

static bool
init_external_functions(void)
{
#define EXTERN(NAME) (external_function_ptr) external :: NAME
#define EXTERNAL0(NAME, RET) external0(EXTERN(NAME), RET)
#define EXTERNAL1(NAME, RET, ARG1) external1(EXTERN(NAME), RET, ARG1)
#define EXTERNAL2(NAME, RET, ARG1, ARG2) external2(EXTERN(NAME), RET, ARG1, ARG2)
#define EXTERNAL3(NAME, RET, ARG1, ARG2, ARG3) external3(EXTERN(NAME), RET, ARG1, ARG2, ARG3)

   register_external_function(EXTERNAL1(sigmoid, FIELD_FLOAT, FIELD_FLOAT));
   register_external_function(EXTERNAL1(randint, FIELD_INT, FIELD_INT));
   register_external_function(EXTERNAL1(normalize, FIELD_LIST_FLOAT, FIELD_LIST_FLOAT));
   register_external_function(EXTERNAL3(damp, FIELD_LIST_FLOAT,
               FIELD_LIST_FLOAT, FIELD_LIST_FLOAT, FIELD_FLOAT));
   register_external_function(EXTERNAL2(divide, FIELD_LIST_FLOAT, FIELD_LIST_FLOAT, FIELD_LIST_FLOAT));
   register_external_function(EXTERNAL2(convolve, FIELD_LIST_FLOAT, FIELD_LIST_FLOAT, FIELD_LIST_FLOAT));
   register_external_function(EXTERNAL2(addfloatlists, FIELD_LIST_FLOAT, FIELD_LIST_FLOAT, FIELD_LIST_FLOAT));
   register_external_function(EXTERNAL1(intlistlength, FIELD_INT, FIELD_LIST_INT));
   register_external_function(EXTERNAL2(intlistdiff, FIELD_LIST_INT, FIELD_LIST_INT, FIELD_LIST_INT));
   register_external_function(EXTERNAL2(intlistnth, FIELD_INT, FIELD_LIST_INT, FIELD_INT));
	register_external_function(EXTERNAL2(concatenate, FIELD_STRING, FIELD_STRING, FIELD_STRING));
	register_external_function(EXTERNAL1(str2float, FIELD_FLOAT, FIELD_STRING));
	register_external_function(EXTERNAL1(str2int, FIELD_INT, FIELD_STRING));
	register_external_function(EXTERNAL2(nodelistremove, FIELD_LIST_NODE, FIELD_LIST_NODE, FIELD_NODE));
	register_external_function(EXTERNAL1(wastetime, FIELD_INT, FIELD_INT));
	register_external_function(EXTERNAL2(truncate, FIELD_FLOAT, FIELD_FLOAT, FIELD_INT));
	register_external_function(EXTERNAL1(float2int, FIELD_INT, FIELD_FLOAT));
	register_external_function(EXTERNAL1(int2str, FIELD_STRING, FIELD_INT));
	register_external_function(EXTERNAL1(float2str, FIELD_STRING, FIELD_FLOAT));
   register_external_function(EXTERNAL3(intlistsub, FIELD_LIST_INT, FIELD_LIST_INT, FIELD_INT, FIELD_INT));
   register_external_function(EXTERNAL2(intlistappend, FIELD_LIST_INT, FIELD_LIST_INT, FIELD_LIST_INT));
   register_external_function(EXTERNAL1(str2intlist, FIELD_LIST_INT, FIELD_STRING));
   register_external_function(EXTERNAL2(filecountwords, FIELD_INT, FIELD_STRING, FIELD_INT));
   register_external_function(EXTERNAL2(residual, FIELD_FLOAT, FIELD_LIST_FLOAT, FIELD_LIST_FLOAT));
   register_external_function(EXTERNAL1(nodelistlength, FIELD_INT, FIELD_LIST_NODE));
   register_external_function(EXTERNAL2(nodelistcount, FIELD_INT, FIELD_LIST_NODE, FIELD_NODE));
   register_external_function(EXTERNAL2(nodelistappend, FIELD_LIST_NODE, FIELD_LIST_NODE, FIELD_LIST_NODE));
   register_external_function(EXTERNAL1(node_priority, FIELD_FLOAT, FIELD_NODE));
   register_external_function(EXTERNAL1(nodelistreverse, FIELD_LIST_NODE, FIELD_LIST_NODE));
   register_external_function(EXTERNAL1(nodelistlast, FIELD_NODE, FIELD_LIST_NODE));
   register_external_function(EXTERNAL1(cpu_id, FIELD_INT, FIELD_NODE));
   register_external_function(EXTERNAL1(node2int, FIELD_INT, FIELD_NODE));
   register_external_function(EXTERNAL2(intpower, FIELD_INT, FIELD_INT, FIELD_INT));
   register_external_function(EXTERNAL1(intlistsort, FIELD_LIST_INT, FIELD_LIST_INT));
   register_external_function(EXTERNAL1(intlistremoveduplicates, FIELD_LIST_INT, FIELD_LIST_INT));
   register_external_function(EXTERNAL2(degeneratevector, FIELD_LIST_INT, FIELD_INT, FIELD_INT));
   register_external_function(EXTERNAL2(demergemessages, FIELD_LIST_INT, FIELD_LIST_INT, FIELD_LIST_INT));
   register_external_function(EXTERNAL2(intlistequal, FIELD_INT, FIELD_LIST_INT, FIELD_LIST_INT));

   atexit(cleanup_externals);
   
   return true;
}

}
