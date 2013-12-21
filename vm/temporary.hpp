
#ifndef VM_TEMPORARY_HPP
#define VM_TEMPORARY_HPP

#include "mem/base.hpp"
#include "vm/program.hpp"
#include "vm/predicate.hpp"
#include "db/tuple.hpp"
#include "vm/rule_matcher.hpp"

namespace vm
{

class temporary_store: public mem::base
{
   public:
      vm::tuple_list *incoming;
      db::simple_tuple_list incoming_persistent_tuples;
      db::simple_tuple_list incoming_action_tuples;

      vm::tuple_list *generated;
      db::simple_tuple_list action_tuples;
      db::simple_tuple_list persistent_tuples;
      size_t num_lists;

      bool *rules;
      size_t size_rules;
      bool *predicates;
      size_t size_predicates;

      vm::rule_matcher matcher;

      inline vm::tuple_list* get_generated(const vm::predicate_id p)
      {
         assert(p < num_lists);
         return generated + p;
      }

      inline vm::tuple_list* get_incoming(const vm::predicate_id p)
      {
         assert(p < num_lists);
         return incoming + p;
      }

      inline void add_incoming(vm::tuple *tpl)
      {
         get_incoming(tpl->get_predicate_id())->push_back(tpl);
      }

      inline void register_fact(db::simple_tuple *stpl)
      {
         register_tuple_fact(stpl->get_tuple(), stpl->get_count());
      }

      inline void register_tuple_fact(vm::tuple *tpl, const vm::ref_count count)
      {
         matcher.register_tuple(tpl, count);
         mark(tpl->get_predicate());
      }

      inline void deregister_fact(db::simple_tuple *stpl)
      {
         deregister_tuple_fact(stpl->get_tuple(), stpl->get_count());
      }

      inline void deregister_tuple_fact(vm::tuple *tpl, const vm::ref_count count)
      {
         matcher.deregister_tuple(tpl, count);
      }

      inline void add_generated(vm::tuple *tpl)
      {
         get_generated(tpl->get_predicate_id())->push_back(tpl);
      }

      inline void add_action_fact(db::simple_tuple *stpl)
      {
         action_tuples.push_back(stpl);
      }

      inline void add_persistent_fact(db::simple_tuple *stpl)
      {
         persistent_tuples.push_back(stpl);
      }

      inline void clear_predicates(void)
      {
         std::fill_n(predicates, size_predicates, false);
      }

      inline void mark(const vm::predicate *pred)
      {
         predicates[pred->get_id()] = true;
      }

      explicit temporary_store(vm::program *prog):
         num_lists(prog->num_predicates()),
         matcher(prog)
      {
         incoming = mem::allocator<vm::tuple_list>().allocate(num_lists);
         generated = mem::allocator<vm::tuple_list>().allocate(num_lists);
         for(size_t i(0); i < num_lists; ++i) {
            mem::allocator<vm::tuple_list>().construct(get_incoming(i));
            mem::allocator<vm::tuple_list>().construct(get_generated(i));
         }
         size_rules = prog->num_rules();
         rules = mem::allocator<bool>().allocate(size_rules);
         size_predicates = prog->num_predicates();
         predicates = mem::allocator<bool>().allocate(size_predicates);
         std::fill_n(predicates, size_predicates, false);
         std::fill_n(rules, size_rules, false);
      }

      ~temporary_store(void)
      {
         for(size_t i(0); i < num_lists; ++i) {
            mem::allocator<vm::tuple_list>().destroy(get_incoming(i));
            mem::allocator<vm::tuple_list>().destroy(get_generated(i));
         }
         mem::allocator<vm::tuple_list>().deallocate(incoming, num_lists);
         mem::allocator<vm::tuple_list>().deallocate(generated, num_lists);
         mem::allocator<bool>().deallocate(rules, size_rules);
         mem::allocator<bool>().deallocate(predicates, size_predicates);
      }
};

}

#endif

