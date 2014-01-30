
#ifndef DB_LINEAR_STORE_HPP
#define DB_LINEAR_STORE_HPP

#include <list>

#include "mem/base.hpp"
#include "db/tuple.hpp"
#include "vm/defs.hpp"
#include "db/intrusive_list.hpp"

namespace db
{

struct linear_store
{
   public:

      typedef intrusive_list<vm::tuple> tuple_list;
      tuple_list *data;
      size_t num_lists;

      inline tuple_list* get_list(const vm::predicate_id p)
      {
         assert(p < num_lists);
         return data + p;
      }

      inline const tuple_list* get_list(const vm::predicate_id p) const
      {
         assert(p < num_lists);
         return data + p;
      }

      inline void add_fact(vm::tuple *tpl)
      {
         get_list(tpl->get_predicate_id())->push_back(tpl);
      }

      explicit linear_store(vm::program *prog):
         num_lists(prog->num_predicates())
      {
         data = mem::allocator<tuple_list>().allocate(num_lists);
         for(size_t i(0); i < num_lists; ++i) {
            mem::allocator<tuple_list>().construct(get_list(i));
         }
      }

      ~linear_store(void)
      {
         for(size_t i(0); i < num_lists; ++i) {
            mem::allocator<tuple_list>().destroy(get_list(i));
         }
         mem::allocator<tuple_list>().deallocate(data, num_lists);
      }
};

}

#endif
