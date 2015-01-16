
#ifndef EXTERNAL_ARRAY_HPP
#define EXTERNAL_ARRAY_HPP

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "vm/external.hpp"

namespace vm
{
   
namespace external
{
   
argument array_init(const argument, const argument, const argument);
argument array_get(const argument, const argument);
argument array_set(const argument, const argument, const argument);
argument array_add(const argument, const argument);
argument array_from_list(const argument, const argument);
argument array_size(const argument);

}

}

#endif

