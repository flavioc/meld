
#ifndef VM_TEMPORARY_HPP
#define VM_TEMPORARY_HPP

#include "mem/base.hpp"
#include "vm/program.hpp"
#include "vm/predicate.hpp"
#include "db/tuple.hpp"
#include "vm/rule_matcher.hpp"
#include "utils/spinlock.hpp"
#include "utils/atomic.hpp"
#include "db/intrusive_list.hpp"

namespace vm
{

struct temporary_store
{
   public:
      bool *predicates;
      size_t size_predicates;

      bool *rules;
      size_t size_rules;

      // incoming linear tuples
      db::intrusive_list<vm::tuple> *incoming;

      // incoming persistent tuples
      db::simple_tuple_list incoming_persistent_tuples;

      // incoming action tuples
      db::simple_tuple_list incoming_action_tuples;

      // generated linear facts
      db::intrusive_list<vm::tuple> *generated;

      // new action facts
      db::simple_tuple_list action_tuples;

      // queue of persistent tuples
      db::simple_tuple_list persistent_tuples;

      size_t num_lists;

      utils::spinlock spin;
      vm::rule_matcher matcher;

      inline db::intrusive_list<vm::tuple>* get_generated(const vm::predicate_id p)
      {
         assert(p < num_lists);
         return generated + p;
      }

      inline db::intrusive_list<vm::tuple>* get_incoming(const vm::predicate_id p)
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
         incoming = mem::allocator<db::intrusive_list<vm::tuple> >().allocate(num_lists);
         generated = mem::allocator<db::intrusive_list<vm::tuple> >().allocate(num_lists);
         for(size_t i(0); i < num_lists; ++i) {
            mem::allocator<db::intrusive_list<vm::tuple> >().construct(get_incoming(i));
            mem::allocator<db::intrusive_list<vm::tuple> >().construct(get_generated(i));
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
            mem::allocator<db::intrusive_list<vm::tuple> >().destroy(get_incoming(i));
            mem::allocator<db::intrusive_list<vm::tuple> >().destroy(get_generated(i));
         }
         mem::allocator<db::intrusive_list<vm::tuple> >().deallocate(incoming, num_lists);
         mem::allocator<db::intrusive_list<vm::tuple> >().deallocate(generated, num_lists);
         mem::allocator<bool>().deallocate(rules, size_rules);
         mem::allocator<bool>().deallocate(predicates, size_predicates);
      }
};

}

#endif

