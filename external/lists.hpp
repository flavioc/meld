
#ifndef EXTERNAL_LISTS_HPP
#define EXTERNAL_LISTS_HPP

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "vm/external.hpp"

namespace vm
{
   
namespace external
{
   
argument listlength(const argument);
argument listappend(const argument, const argument);
argument listreverse(const argument);
argument listlast(const argument);

argument intlistnth(const argument, const argument);
argument intlistdiff(const argument, const argument);
argument intlistsub(const argument, const argument, const argument);
argument intlistsort(const argument);
argument intlistremoveduplicates(const argument);
argument intlistequal(const argument, const argument);
argument str2intlist(const argument);

argument nodelistremove(const argument, const argument);
argument nodelistcount(const argument, const argument);

}

}

#endif

