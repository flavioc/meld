
#ifndef EXTERNAL_CORE_HPP
#define EXTERNAL_CORE_HPP

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "vm/external.hpp"

namespace vm
{
   
namespace external
{
   
argument node_priority(const argument);
argument cpu_id(const argument);
argument cpu_static(const argument);
argument is_static(const argument);
argument is_moving(const argument);
argument partition_vertical(const argument, const argument, const argument, const argument);

}

}

#endif
