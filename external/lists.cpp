
#include <cmath>

#include "runtime/list.hpp"
#include "external/lists.hpp"

using namespace std;
using namespace runtime;

namespace vm
{
namespace external
{
   
argument
intlistlength(EXTERNAL_ARG(ls))
{
   DECLARE_ARG(ls, int_list *);
   int_val total(0);
   int_list *p((int_list *)ls);

   while(!int_list::is_null(p)) {
      total++;
      p = p->get_tail();
   }

   RETURN_ARG(total);
}

static bool inline
has_value(const int_list *_l, int_val v)
{
   int_list *l((int_list*)_l);

   while(!int_list::is_null(l)) {
      if(l->get_head() == v) {
         return true;
      }

      l = l->get_tail();
   }

   return false;
}

argument
intlistdiff(EXTERNAL_ARG(ls1), EXTERNAL_ARG(ls2))
{
   DECLARE_ARG(ls1, int_list *);
   DECLARE_ARG(ls2, int_list *);
   
   int_list *p((int_list*)ls1);

   stack_int_list s;

   while(!int_list::is_null(p)) {

      if(!has_value(ls2, p->get_head()))
         s.push(p->get_head());

      p = p->get_tail();
   }

   int_list *ptr(from_stack_to_list<stack_int_list, int_list>(s));
      
   RETURN_ARG(ptr);
}

argument
intlistnth(EXTERNAL_ARG(ls), EXTERNAL_ARG(v))
{
   DECLARE_ARG(ls, int_list*);
   DECLARE_ARG(v, int_val);

   int_list *p((int_list*)ls);
   int_val count(0);

   while(!int_list::is_null(p)) {

      if(count == v) {
         int_val r(p->get_head());

         RETURN_ARG(r);
      }

      count++;
      p = p->get_tail();
   }

   assert(false);
}

}
}
