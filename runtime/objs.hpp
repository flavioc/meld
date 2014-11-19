
#ifndef RUNTIME_OBJS_HPP
#define RUNTIME_OBJS_HPP

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

void do_increment_runtime(const vm::tuple_field&, const vm::field_type);
void do_decrement_runtime(const vm::tuple_field&, const vm::field_type
#ifdef GC_NODES
      , vm::candidate_gc_nodes&
#endif
      );

inline void increment_runtime_data(const vm::tuple_field& f, const vm::field_type t)
{
   switch(t) {
      case vm::FIELD_NODE:
      case vm::FIELD_STRING:
      case vm::FIELD_STRUCT:
      case vm::FIELD_LIST:
         do_increment_runtime(f, t);
         break;
      default: break;
   }
}

inline void decrement_runtime_data(const vm::tuple_field& f, const vm::field_type t
#ifdef GC_NODES
      , vm::candidate_gc_nodes& gc_nodes
#endif
      )
{
   switch(t) {
      case vm::FIELD_NODE:
      case vm::FIELD_STRING:
      case vm::FIELD_STRUCT:
      case vm::FIELD_LIST:
#ifdef GC_NODES
         do_decrement_runtime(f, t, gc_nodes);
#else
         do_decrement_runtime(f, t);
#endif
         break;
      default: break;
   }
}

#include "runtime/list.hpp"
#include "runtime/struct.hpp"
#include "runtime/string.hpp"

}

#endif
