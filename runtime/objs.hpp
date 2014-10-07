
#ifndef RUNTIME_OBJS_HPP
#define RUNTIME_OBJS_HPP

#include "conf.hpp"

#include <iostream>
#include <string>
#include <stack>
#include <list>

#include "utils/types.hpp"
#include "utils/serialization.hpp"
#include "mem/base.hpp"
#include "vm/types.hpp"
#include "runtime/ref_base.hpp"

#include "vm/defs.hpp"

namespace runtime {

static inline void increment_runtime_data(const vm::tuple_field& f, vm::type *t);
static inline void decrement_runtime_data(const vm::tuple_field& f, vm::type *t);

#include "runtime/list.hpp"
#include "runtime/struct.hpp"
#include "runtime/string.hpp"

static inline void do_increment_runtime(const vm::tuple_field& f)
{
   runtime::ref_base *p((ref_base*)FIELD_PTR(f));

   if(p)
      p->refs++;
}

static inline void do_decrement_runtime(const vm::tuple_field& f, const vm::type* t)
{
   runtime::ref_base *p((ref_base*)FIELD_PTR(f));

   if(p) {
      assert(p->refs > 0);
      p->refs--;
      if(p->refs == 0) {
         switch(t->get_type()) {
            case vm::FIELD_LIST: ((runtime::cons*)p)->destroy(); break;
            case vm::FIELD_STRING: ((runtime::rstring*)p)->destroy(); break;
            case vm::FIELD_STRUCT: ((runtime::struct1*)p)->destroy(); break;
            default: assert(false);
         }
      }
   }
}

static inline void increment_runtime_data(const vm::tuple_field& f, vm::type *t)
{
   if(t->is_ref())
      do_increment_runtime(f);
}

static inline void decrement_runtime_data(const vm::tuple_field& f, vm::type *t)
{
   switch(t->get_type()) {
      case vm::FIELD_INT:
      case vm::FIELD_FLOAT:
      case vm::FIELD_NODE:
      case vm::FIELD_STRING:
         break;
      case vm::FIELD_LIST: {
         cons *l(FIELD_CONS(f));

         if(l)
            l->dec_refs();
      }
      break;
      case vm::FIELD_STRUCT:
         FIELD_STRUCT(f)->dec_refs();
         break;
      default: assert(false);
   }
}

}

#endif
