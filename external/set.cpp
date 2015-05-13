#include "runtime/objs.hpp"
#include "external/set.hpp"
#include "vm/all.hpp"

using namespace std;
using namespace runtime;

namespace vm
{
namespace external
{
   
argument
set_size(EXTERNAL_ARG(s))
{
   DECLARE_SET(s);
   const vm::int_val size(s->get_size());
   RETURN_INT(size);
}

argument
set_exists(EXTERNAL_ARG(s), EXTERNAL_ARG(item), EXTERNAL_ARG(typ))
{
   DECLARE_SET(s);
   DECLARE_ANY(item);
   DECLARE_TYPE(typ);
   (void)typ;

   RETURN_BOOL(s->exists(item));
}

argument
set_add(EXTERNAL_ARG(s), EXTERNAL_ARG(item), EXTERNAL_ARG(typ))
{
   DECLARE_SET(s);
   DECLARE_ANY(item);
   DECLARE_TYPE(typ);

   runtime::set *n(runtime::set::mutate_add(s, typ, item));

   RETURN_SET(n);
}

argument
set_from_list(EXTERNAL_ARG(ls), EXTERNAL_ARG(typ))
{
   DECLARE_SET(ls);
   DECLARE_TYPE(typ);

   vector<vm::tuple_field, mem::allocator<vm::tuple_field>> vals;
   runtime::cons *p((runtime::cons*)ls);
   while(!runtime::cons::is_null(p)) {
      vals.push_back(p->get_head());
      p = p->get_tail();
   }

   runtime::set *n(runtime::set::create_from_vector(typ, vals));
   RETURN_SET(n);
}

}

}
