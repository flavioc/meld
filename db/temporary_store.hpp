
#ifndef DB_TEMPORARY_STORE_HPP
#define DB_TEMPORARY_STORE_HPP

#include <unordered_map>

#include "mem/base.hpp"
#include "vm/program.hpp"
#include "vm/predicate.hpp"
#include "vm/full_tuple.hpp"
#include "utils/intrusive_list.hpp"
#include "vm/bitmap.hpp"

namespace vm
{

//#define UNIQUE_INCOMING_LIST

struct temporary_store
{
   public:

      typedef utils::intrusive_list<vm::tuple> tuple_list;

      // incoming linear tuples
#ifdef UNIQUE_INCOMING_LIST
      tuple_list incoming;
#else
      tuple_list *incoming;
#endif

      // incoming persistent tuples
      full_tuple_list incoming_persistent_tuples;

      // incoming action tuples
      full_tuple_list incoming_action_tuples;

      // generated linear facts
      tuple_list *generated;

      // new action facts
      full_tuple_list action_tuples;

      // queue of persistent tuples
      full_tuple_list persistent_tuples;

      inline tuple_list* get_generated(const vm::predicate_id p)
      {
         return generated + p;
      }

#ifndef UNIQUE_INCOMING_LIST
      inline tuple_list* get_incoming(const vm::predicate_id p)
      {
         return incoming + p;
      }
#endif
      inline void add_incoming(vm::tuple *tpl, vm::predicate *pred)
      {
#ifdef UNIQUE_INCOMING_LIST
         (void)pred;
         incoming.push_back(tpl);
#else
         tuple_list *ls(get_incoming(pred->get_id()));

         ls->push_back(tpl);
#endif
      }

      inline void add_generated(vm::tuple *tpl, vm::predicate *pred)
      {
         tuple_list *ls(get_generated(pred->get_id()));
         ls->push_back(tpl);
      }

      inline void add_action_fact(full_tuple *stpl)
      {
         action_tuples.push_back(stpl);
      }

      inline void add_persistent_fact(full_tuple *stpl)
      {
         persistent_tuples.push_back(stpl);
      }

      explicit temporary_store(void)
      {
#ifndef UNIQUE_INCOMING_LIST
         incoming = mem::allocator<tuple_list>().allocate(theProgram->num_predicates());
         for(size_t i(0); i < theProgram->num_predicates(); ++i)
            mem::allocator<tuple_list>().construct(incoming + i);
#endif
         generated = mem::allocator<tuple_list>().allocate(theProgram->num_predicates());
         for(size_t i(0); i < theProgram->num_predicates(); ++i)
            mem::allocator<tuple_list>().construct(generated + i);
      }

      ~temporary_store(void)
      {
#ifndef UNIQUE_INCOMING_LIST
         for(size_t i(0); i < theProgram->num_predicates(); ++i)
            mem::allocator<tuple_list>().destroy(incoming + i);
         mem::allocator<tuple_list>().deallocate(incoming, theProgram->num_predicates());
#endif
         for(size_t i(0); i < theProgram->num_predicates(); ++i)
            mem::allocator<tuple_list>().destroy(generated + i);
         mem::allocator<tuple_list>().deallocate(generated, theProgram->num_predicates());
      }
};

}

#endif

