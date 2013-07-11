
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
argument intlistsort(const argument);
argument intlistremoveduplicates(const argument);
argument intlistequal(const argument, const argument);
argument str2intlist(const argument);

argument nodelistremove(const argument, const argument);
argument nodelistlength(const argument);
argument nodelistcount(const argument, const argument);
argument nodelistappend(const argument, const argument);
argument nodelistreverse(const argument);
argument nodelistlast(const argument);

}

}

#endif

