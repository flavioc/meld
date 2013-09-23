
#include "runtime/common.hpp"
#include "runtime/list.hpp"
#include "runtime/struct.hpp"
#include "vm/defs.hpp"

using namespace vm;

namespace runtime
{

void
increment_runtime_data(const tuple_field& f, type *t)
{
   assert(t);
   switch(t->get_type()) {
      case FIELD_LIST: {
         cons *l(FIELD_CONS(f));

         if(!runtime::cons::is_null(l)) {
            l->inc_refs();
         }
      }
      break;
      case FIELD_STRUCT: {
         struct1 *s(FIELD_STRUCT(f));

         s->inc_refs();
      }
      break;
      case FIELD_INT:
      case FIELD_FLOAT:
      case FIELD_NODE:
      case FIELD_STRING:
         break;
      default: assert(false);
   }
}

void
decrement_runtime_data(const tuple_field& f, type *t)
{
   assert(t);
   switch(t->get_type()) {
      case FIELD_LIST: {
         cons *l(FIELD_CONS(f));

         if(!runtime::cons::is_null(l)) {
            l->dec_refs();
         }
      }
      break;
      case FIELD_STRUCT: {
         struct1 *s(FIELD_STRUCT(f));

         s->dec_refs();
      }
      break;
      case FIELD_INT:
      case FIELD_FLOAT:
      case FIELD_NODE:
      case FIELD_STRING:
         break;
      default: assert(false);
   }
}

}
