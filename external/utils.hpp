
#ifndef EXTERNAL_UTILS_HPP
#define EXTERNAL_UTILS_HPP

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "vm/external.hpp"

namespace vm
{
namespace external
{

argument randint(const argument);
argument str2float(const argument);
argument str2int(const argument);
argument int2str(const argument);
argument float2str(const argument);
argument truncate(const argument, const argument);
argument float2int(const argument);
argument wastetime(const argument);
argument node2int(const argument);
   
}
}

#endif
