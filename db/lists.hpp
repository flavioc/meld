
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

      db::simple_tuple_list *data;
      size_t num_lists;

      inline db::simple_tuple_list* get_list(const vm::predicate_id p)
      {
         assert(p < num_lists);
         return data + p;
      }

      inline const db::simple_tuple_list* get_list(const vm::predicate_id p) const
      {
         assert(p < num_lists);
         return data + p;
      }

      inline void add_fact(db::simple_tuple *stpl)
      {
         get_list(stpl->get_predicate_id())->push_back(stpl);
      }

      explicit lists(vm::program *prog):
         num_lists(prog->num_predicates())
      {
         data = mem::allocator<db::simple_tuple_list>().allocate(num_lists);
         for(size_t i(0); i < num_lists; ++i) {
            mem::allocator<db::simple_tuple_list>().construct(get_list(i));
         }
      }

      ~lists(void)
      {
         for(size_t i(0); i < num_lists; ++i) {
            mem::allocator<db::simple_tuple_list>().destroy(get_list(i));
         }
         mem::allocator<db::simple_tuple_list>().deallocate(data, num_lists);
      }
};

}

#endif
