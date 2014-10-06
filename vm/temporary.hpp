
#ifndef VM_TEMPORARY_HPP
#define VM_TEMPORARY_HPP

#include <unordered_map>

#include "mem/base.hpp"
#include "vm/program.hpp"
#include "vm/predicate.hpp"
#include "db/tuple.hpp"
#include "vm/rule_matcher.hpp"
#include "utils/atomic.hpp"
#include "utils/mutex.hpp"
#include "utils/intrusive_list.hpp"
#include "vm/bitmap.hpp"

namespace vm
{

struct temporary_store
{
   public:

      typedef utils::intrusive_list<vm::tuple> tuple_list;

      // incoming linear tuples
      tuple_list *incoming;

      typedef std::unordered_map<vm::predicate_id, tuple_list*, std::hash<vm::predicate_id>,
              std::equal_to<vm::predicate_id>, mem::allocator< std::pair<const vm::predicate_id, tuple_list*> > > list_map;

      // incoming persistent tuples
      db::simple_tuple_list incoming_persistent_tuples;

      // incoming action tuples
      db::simple_tuple_list incoming_action_tuples;

      // generated linear facts
      list_map generated;

      // new action facts
      db::simple_tuple_list action_tuples;

      // queue of persistent tuples
      db::simple_tuple_list persistent_tuples;

      utils::mutex spin;
      vm::rule_matcher matcher;

      inline tuple_list* get_generated(const vm::predicate_id p)
      {
         assert(p < theProgram->num_predicates());
         list_map::iterator it(generated.find(p));
         if(it == generated.end())
            return NULL;
         return it->second;
      }

      inline tuple_list* get_incoming(const vm::predicate_id p)
      {
         return incoming + p;
      }

      inline void add_incoming(vm::tuple *tpl, vm::predicate *pred)
      {
         tuple_list *ls(get_incoming(pred->get_id()));

         ls->push_back(tpl);
      }

      inline void register_fact(db::simple_tuple *stpl)
      {
         register_tuple_fact(stpl->get_predicate(), stpl->get_count());
      }

      inline void register_tuple_fact(vm::predicate *pred, const vm::ref_count count)
      {
         matcher.register_tuple(pred, count);
      }

      inline void deregister_fact(db::simple_tuple *stpl)
      {
         deregister_tuple_fact(stpl->get_predicate(), stpl->get_count());
      }

      inline void deregister_tuple_fact(vm::predicate *pred, const vm::ref_count count)
      {
         matcher.deregister_tuple(pred, count);
      }

      inline tuple_list* create_generated(const vm::predicate_id p)
      {
         tuple_list *ls(mem::allocator<tuple_list>().allocate(1));
         mem::allocator<tuple_list>().construct(ls);
         generated.insert(std::make_pair(p, ls));
         return ls;
      }

      inline void add_generated(vm::tuple *tpl, vm::predicate *pred)
      {
         tuple_list *ls(get_generated(pred->get_id()));
         if(ls == NULL)
            ls = create_generated(pred->get_id());
         ls->push_back(tpl);
      }

      inline void add_action_fact(db::simple_tuple *stpl)
      {
         action_tuples.push_back(stpl);
      }

      inline void add_persistent_fact(db::simple_tuple *stpl)
      {
         persistent_tuples.push_back(stpl);
      }

      explicit temporary_store(void)
      {
         incoming = mem::allocator<tuple_list>().allocate(theProgram->num_predicates());
         for(size_t i(0); i < theProgram->num_predicates(); ++i)
            mem::allocator<tuple_list>().construct(incoming + i);
      }

      ~temporary_store(void)
      {
         for(size_t i(0); i < theProgram->num_predicates(); ++i)
            mem::allocator<tuple_list>().destroy(incoming + i);
         mem::allocator<tuple_list>().deallocate(incoming, theProgram->num_predicates());
         for(list_map::iterator it(generated.begin()), end(generated.end()); it != end; ++it) {
            tuple_list *ls(it->second);
            mem::allocator<tuple_list>().destroy(ls);
            mem::allocator<tuple_list>().deallocate(ls, 1);
         }
      }
};

}

#endif

