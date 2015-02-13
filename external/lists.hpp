
#ifndef EXTERNAL_LISTS_HPP
#define EXTERNAL_LISTS_HPP

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "vm/external.hpp"

namespace vm
{
   
namespace external
{
   
argument listcount(const argument, const argument);
argument lexists(const argument, const argument);
argument lexistss(const argument, const argument);
argument llength(const argument);
argument lappend(const argument, const argument);
argument lreverse(const argument);
argument listlast(const argument);
argument lnth(const argument, const argument);
argument lsort(const argument);
argument lremoveduplicates(const argument);
argument listremove(const argument, const argument);

argument intlistdiff(const argument, const argument);
argument intlistsub(const argument, const argument, const argument);
argument intlistequal(const argument, const argument);
argument str2intlist(const argument);

}

}

#endif

