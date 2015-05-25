
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
array_init(EXTERNAL_ARG(size), EXTERNAL_ARG(item), EXTERNAL_ARG(typ))
{
   DECLARE_INT(size);
   DECLARE_ANY(item);
   DECLARE_TYPE(typ);

   runtime::array* x(runtime::array::create_fill(typ, size, item));
   RETURN_ARRAY(x);
}

argument
array_add(EXTERNAL_ARG(arr), EXTERNAL_ARG(item), EXTERNAL_ARG(typ))
{
   DECLARE_ARRAY(arr);
   DECLARE_ANY(item);
   DECLARE_TYPE(typ);

   RETURN_ARRAY(runtime::array::mutate_add(arr, typ, item));
}

argument
array_from_list(EXTERNAL_ARG(ls), EXTERNAL_ARG(typ))
{
   DECLARE_LIST(ls);
   DECLARE_TYPE(typ);
   if(runtime::cons::is_null(ls))
      RETURN_ARRAY(runtime::array::create_empty());

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

argument
array_exists(EXTERNAL_ARG(a), EXTERNAL_ARG(item), EXTERNAL_ARG(ty))
{
   DECLARE_ARRAY(a);
   DECLARE_ANY(item);
   DECLARE_TYPE(ty);

   const bool exists(a->exists(item, ty));

   RETURN_BOOL(exists);
}

}
}
