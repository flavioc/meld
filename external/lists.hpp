
#ifndef EXTERNAL_LISTS_HPP
#define EXTERNAL_LISTS_HPP

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "vm/external.hpp"
#include "runtime/objs.hpp"

namespace vm
{
   
namespace external
{
   
argument listcount(const argument, const argument, const argument);

static inline argument
lexists(EXTERNAL_ARG(ls), EXTERNAL_ARG(i), EXTERNAL_ARG(ty))
{
   DECLARE_LIST(ls);
   DECLARE_TYPE(ty);
   runtime::cons *p((runtime::cons *)ls);
   if(runtime::cons::is_null(p))
      RETURN_BOOL(false);

   switch(ty->get_type()) {
      case FIELD_INT: {
            DECLARE_INT(i);
            while(!runtime::cons::is_null(p)) {
               if(FIELD_INT(p->get_head()) == i)
                  RETURN_BOOL(true);
               p = p->get_tail();
            }
         }
         break;
      case FIELD_FLOAT: {
            DECLARE_FLOAT(i);
            while(!runtime::cons::is_null(p)) {
               if(FIELD_FLOAT(p->get_head()) == i)
                  RETURN_BOOL(true);
               p = p->get_tail();
            }
         }
         break;
      case FIELD_NODE: {
            DECLARE_NODE(i);
            while(!runtime::cons::is_null(p)) {
               if(FIELD_NODE(p->get_head()) == i)
                  RETURN_BOOL(true);
               p = p->get_tail();
            }
         }
         break;
      default:
         std::cerr << "cannot count types in listexists\n";
         abort();
         break;
   }

   RETURN_BOOL(false);
}

static inline argument
llength(EXTERNAL_ARG(ls))
{
	DECLARE_LIST(ls);
   int_val total(0);
   runtime::cons *p((runtime::cons *)ls);

   while(!runtime::cons::is_null(p)) {
      total++;
      p = p->get_tail();
   }

	RETURN_INT(total);
}


argument lexistss(const argument, const argument, const argument);
argument lappend(const argument, const argument, const argument);
argument lreverse(const argument, const argument);
argument listlast(const argument);
argument lnth(const argument, const argument);
argument lsort(const argument, const argument);
argument lremoveduplicates(const argument, const argument);
argument lremove(const argument, const argument, const argument);
argument ltake(const argument, const argument);

argument intlistdiff(const argument, const argument);
argument intlistsub(const argument, const argument, const argument);
argument intlistequal(const argument, const argument);
argument str2intlist(const argument);

}

}

#endif

