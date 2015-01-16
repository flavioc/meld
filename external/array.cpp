
#include "external/array.hpp"
#include "runtime/objs.hpp"
#include "vm/all.hpp"

using namespace std;
using namespace vm;

namespace vm
{
namespace external
{

argument
array_init(EXTERNAL_ARG(size), EXTERNAL_ARG(item), EXTERNAL_ARG(type))
{
   DECLARE_INT(size);
   DECLARE_ANY(item);
   DECLARE_INT(type);
   vm::type *typ(theProgram->get_type(type));

   runtime::array* x(runtime::array::create_fill(typ, size, item));
   RETURN_ARRAY(x);
}

argument
array_get(EXTERNAL_ARG(arr), EXTERNAL_ARG(i))
{
   DECLARE_ARRAY(arr);
   DECLARE_INT(i);

   return arr->get_item(i);
}

argument
array_set(EXTERNAL_ARG(arr), EXTERNAL_ARG(i), EXTERNAL_ARG(item))
{
   DECLARE_ARRAY(arr);
   DECLARE_INT(i);
   DECLARE_ANY(item);

   RETURN_ARRAY(runtime::array::mutate(arr, i, item));
}

argument
array_add(EXTERNAL_ARG(arr), EXTERNAL_ARG(item))
{
   DECLARE_ARRAY(arr);
   DECLARE_ANY(item);

   RETURN_ARRAY(runtime::array::mutate_add(arr, item));
}

argument
array_from_list(EXTERNAL_ARG(ls), EXTERNAL_ARG(t))
{
   DECLARE_LIST(ls);
   DECLARE_INT(t);
   vm::type *typ(theProgram->get_type(t));
   if(runtime::cons::is_null(ls))
      RETURN_ARRAY(runtime::array::create_empty(typ));

   vector<vm::tuple_field, mem::allocator<vm::tuple_field>> vals;
   runtime::cons *p((runtime::cons*)ls);
   while(!runtime::cons::is_null(p)) {
      vals.push_back(p->get_head());
      p = p->get_tail();
   }
   RETURN_ARRAY(runtime::array::create_from_vector(typ, vals));
}

argument
array_size(EXTERNAL_ARG(a))
{
   DECLARE_ARRAY(a);

   RETURN_INT(a->get_size());
}

}
}
