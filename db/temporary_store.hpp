
#ifndef DB_TEMPORARY_STORE_HPP
#define DB_TEMPORARY_STORE_HPP

#include <unordered_map>

#include "mem/base.hpp"
#include "vm/program.hpp"
#include "vm/all.hpp"
#include "vm/predicate.hpp"
#include "vm/full_tuple.hpp"
#include "utils/intrusive_list.hpp"
#include "vm/bitmap.hpp"

namespace db {

struct temporary_store {
   public:
   using tuple_list = vm::tuple_list;

// incoming linear tuples
#ifdef COMPILED
   tuple_list incoming[COMPILED_NUM_LINEAR];
#else
   tuple_list *incoming{nullptr};
#endif

   // incoming persistent tuples
   vm::full_tuple_list incoming_persistent_tuples;

   // incoming action tuples
   vm::full_tuple_list incoming_action_tuples;

   inline tuple_list *get_incoming(const vm::predicate_id p) {
      assert(p < vm::theProgram->num_linear_predicates());
      return incoming + p;
   }
   inline void add_incoming(vm::tuple *tpl, const vm::predicate *pred) {
      tuple_list *ls(get_incoming(pred->get_linear_id()));

      ls->push_back(tpl);
   }
   inline void add_incoming_list(vm::tuple_list &other, vm::predicate *pred) {
      tuple_list *ls(get_incoming(pred->get_linear_id()));

      ls->splice_back(other);
   }

   explicit temporary_store(void) {
#ifndef COMPILED
      if (vm::theProgram->num_linear_predicates() > 0) {
         incoming = mem::allocator<tuple_list>().allocate(
             vm::theProgram->num_linear_predicates());
         for (size_t i(0); i < vm::theProgram->num_linear_predicates(); ++i)
            mem::allocator<tuple_list>().construct(get_incoming(i));
      }
#endif
   }

   ~temporary_store(void) {
#ifndef COMPILED
      if (vm::theProgram->num_linear_predicates() > 0) {
         for (size_t i(0); i < vm::theProgram->num_linear_predicates(); ++i)
            mem::allocator<tuple_list>().destroy(get_incoming(i));
         mem::allocator<tuple_list>().deallocate(
             incoming, vm::theProgram->num_linear_predicates());
      }
#endif
   }
};
}

#endif
