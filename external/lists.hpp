
#ifndef EXTERNAL_LISTS_HPP
#define EXTERNAL_LISTS_HPP

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "vm/external.hpp"

namespace vm
{
   
namespace external
{
   
argument intlistlength(const argument);
argument intlistdiff(const argument, const argument);
argument intlistnth(const argument, const argument);
argument intlistsub(const argument, const argument, const argument);
argument intlistappend(const argument, const argument);

argument nodelistremove(const argument, const argument);

}

}

#endif

