
#ifndef EXTERNAL_ARRAY_HPP
#define EXTERNAL_ARRAY_HPP

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "vm/all.hpp"
#include "vm/external.hpp"
#include "runtime/objs.hpp"

namespace vm
{
   
namespace external
{
   
argument array_init(const argument, const argument, const argument);

static inline argument
array_get(EXTERNAL_ARG(arr), EXTERNAL_ARG(i))
{
   DECLARE_ARRAY(arr);
   DECLARE_INT(i);

   return arr->get_item(i);
}

static inline argument
array_set(EXTERNAL_ARG(arr), EXTERNAL_ARG(i), EXTERNAL_ARG(item), EXTERNAL_ARG(typ))
{
   DECLARE_ARRAY(arr);
   DECLARE_INT(i);
   DECLARE_ANY(item);
   DECLARE_TYPE(typ);

   RETURN_ARRAY(runtime::array::mutate(arr, typ, i, item));
}

argument array_add(const argument, const argument, const argument);
argument array_from_list(const argument, const argument);
argument array_size(const argument);
argument array_exists(const argument, const argument, const argument);

}

}

#endif

