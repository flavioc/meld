
#ifndef VM_TEMPORARY_HPP
#define VM_TEMPORARY_HPP

#include "mem/base.hpp"
#include "vm/program.hpp"
#include "db/tuple.hpp"

namespace vm
{

class temporary_store: public mem::base
{
   public:
      db::simple_tuple_list *lists;
      db::simple_tuple_list *generated;
      db::simple_tuple_list *action_tuples;
      db::simple_tuple_list *persistent_tuples;
      size_t num_lists;
      size_t size;

      inline db::simple_tuple_list* get_generated(const vm::predicate_id p)
      {
         assert(p < num_lists);
         return generated + p;
      }

      inline db::simple_tuple_list* get_list(const vm::predicate_id p)
      {
         assert(p < num_lists);
         return lists + p;
      }

      inline bool has_data(void) const
      {
         return size > 0;
      }

      inline void add_fact(db::simple_tuple *stpl)
      {
         size++;
         get_list(stpl->get_predicate_id())->push_back(stpl);
      }

      inline void add_generated(db::simple_tuple *stpl)
      {
         get_generated(stpl->get_predicate_id())->push_back(stpl);
      }

      inline void add_action_fact(db::simple_tuple *stpl)
      {
         size++;
         action_tuples->push_back(stpl);
      }

      inline void add_persistent_fact(db::simple_tuple *stpl)
      {
         size++;
         persistent_tuples->push_back(stpl);
      }

      explicit temporary_store(vm::program *prog):
         num_lists(prog->num_predicates()),
         size(0)
      {
         lists = mem::allocator<db::simple_tuple_list>().allocate(num_lists);
         generated = mem::allocator<db::simple_tuple_list>().allocate(num_lists);
         action_tuples = mem::allocator<db::simple_tuple_list>().allocate(1);
         persistent_tuples = mem::allocator<db::simple_tuple_list>().allocate(1);
         for(size_t i(0); i < num_lists; ++i) {
            mem::allocator<db::simple_tuple_list>().construct(get_list(i));
            mem::allocator<db::simple_tuple_list>().construct(get_generated(i));
         }
         mem::allocator<db::simple_tuple_list>().construct(action_tuples);
         mem::allocator<db::simple_tuple_list>().construct(persistent_tuples);
      }

      ~temporary_store(void)
      {
         for(size_t i(0); i < num_lists; ++i) {
            mem::allocator<db::simple_tuple_list>().destroy(get_list(i));
            mem::allocator<db::simple_tuple_list>().destroy(get_generated(i));
         }
         mem::allocator<db::simple_tuple_list>().destroy(action_tuples);
         mem::allocator<db::simple_tuple_list>().destroy(persistent_tuples);
         mem::allocator<db::simple_tuple_list>().deallocate(lists, num_lists);
         mem::allocator<db::simple_tuple_list>().deallocate(generated, num_lists);
         mem::allocator<db::simple_tuple_list>().deallocate(action_tuples, 1);
         mem::allocator<db::simple_tuple_list>().deallocate(persistent_tuples, 1);
      }
};

}

#endif

