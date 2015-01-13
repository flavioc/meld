
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
argument listexists(const argument, const argument);
argument listlength(const argument);
argument listappend(const argument, const argument);
argument listreverse(const argument);
argument listlast(const argument);
argument listnth(const argument, const argument);
argument listsort(const argument);
argument listremoveduplicates(const argument);
argument listremove(const argument, const argument);

argument intlistdiff(const argument, const argument);
argument intlistsub(const argument, const argument, const argument);
argument intlistequal(const argument, const argument);
argument str2intlist(const argument);

}

}

#endif

