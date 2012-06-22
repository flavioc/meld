
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
   
}
}

#endif
