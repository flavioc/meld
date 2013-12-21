
#ifndef DB_LISTS_HPP
#define DB_LISTS_HPP

#include <list>

#include "mem/base.hpp"
#include "db/tuple.hpp"
#include "vm/defs.hpp"

namespace db
{

class lists: public mem::base
{
   public:

      vm::tuple_list *data;
      size_t num_lists;

      inline vm::tuple_list* get_list(const vm::predicate_id p)
      {
         assert(p < num_lists);
         return data + p;
      }

      inline const vm::tuple_list* get_list(const vm::predicate_id p) const
      {
         assert(p < num_lists);
         return data + p;
      }

      inline void add_fact(vm::tuple *tpl)
      {
         get_list(tpl->get_predicate_id())->push_back(tpl);
      }

      explicit lists(vm::program *prog):
         num_lists(prog->num_predicates())
      {
         data = mem::allocator<vm::tuple_list>().allocate(num_lists);
         for(size_t i(0); i < num_lists; ++i) {
            mem::allocator<vm::tuple_list>().construct(get_list(i));
         }
      }

      ~lists(void)
      {
         for(size_t i(0); i < num_lists; ++i) {
            mem::allocator<vm::tuple_list>().destroy(get_list(i));
         }
         mem::allocator<vm::tuple_list>().deallocate(data, num_lists);
      }
};

}

#endif
