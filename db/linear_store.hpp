
#ifndef DB_LINEAR_STORE_HPP
#define DB_LINEAR_STORE_HPP

#include <list>
#include <set>
#include <iostream>

#include "mem/base.hpp"
#include "db/tuple.hpp"
#include "vm/defs.hpp"
#include "db/intrusive_list.hpp"
#include "db/hash_table.hpp"

#define EXPAND_HASH_TABLES 1

namespace db
{

struct linear_store
{
   public:

      typedef intrusive_list<vm::tuple> tuple_list;
      tuple_list *lists;
      size_t num;

      hash_table *tables;
      vm::program *prog;

#ifdef EXPAND_HASH_TABLES
      typedef std::set<hash_table*, std::less<hash_table*>, mem::allocator<hash_table*> > set_tables_expand;
      set_tables_expand tables_to_expand;
#endif

   private:

      inline hash_table* get_table(const vm::predicate_id p)
      {
         assert(p < num);
         return tables + p;
      }

      inline hash_table* get_table(const vm::predicate_id p) const
      {
         assert(p < num);
         return tables + p;
      }

      inline tuple_list* get_list(const vm::predicate_id p)
      {
         assert(p < num);
         return lists + p;
      }

      inline const tuple_list* get_list(const vm::predicate_id p) const
      {
         assert(p < num);
         return lists + p;
      }

   public:

      inline tuple_list* get_linked_list(const vm::predicate_id p) { assert(!prog->get_predicate(p)->is_hash_table()); return get_list(p); }
      inline const tuple_list* get_linked_list(const vm::predicate_id p) const { assert(!prog->get_predicate(p)->is_hash_table()); return get_list(p); }

      inline hash_table* get_hash_table(const vm::predicate_id p) { assert(prog->get_predicate(p)->is_hash_table()); return get_table(p); }
      inline const hash_table* get_hash_table(const vm::predicate_id p) const { assert(prog->get_predicate(p)->is_hash_table()); return get_table(p); }

      inline void add_fact(vm::tuple *tpl)
      {
         const vm::predicate *pred(tpl->get_predicate());
         if(pred->is_hash_table()) {
            hash_table *table(get_table(pred->get_id()));
            table->insert(tpl);
#ifdef EXPAND_HASH_TABLES
            if(table->must_check_hash_table())
               tables_to_expand.insert(table);
#endif
         } else
            get_list(pred->get_id())->push_back(tpl);
      }

      inline void increment_database(vm::predicate *pred, tuple_list *ls)
      {
         if(pred->is_hash_table()) {
            hash_table *table(get_table(pred->get_id()));
            for(tuple_list::iterator it(ls->begin()), end(ls->end()); it != end; ++it)
               table->insert(*it);
            ls->clear();
#ifdef EXPAND_HASH_TABLES
            if(table->must_check_hash_table())
               tables_to_expand.insert(table);
#endif
         } else
            get_list(pred->get_id())->splice_back(*ls);
      }

      inline void improve_index(void)
      {
#ifdef EXPAND_HASH_TABLES
         for(set_tables_expand::iterator it(tables_to_expand.begin()), end(tables_to_expand.end());
               it != end; ++it)
         {
            hash_table *table(*it);
            if(table->too_crowded()) {
               table->expand();
               //std::cout << "Expanding... " << table->get_table_size() << "\n";
            }
            table->reset_added();
         }
         tables_to_expand.clear();
#endif
      }

      explicit linear_store(vm::program *_prog):
         num(_prog->num_predicates()),
         prog(_prog)
      {
         lists = mem::allocator<tuple_list>().allocate(num);
         tables = mem::allocator<hash_table>().allocate(num);
         for(size_t i(0); i < num; ++i) {
            vm::predicate *pred(prog->get_predicate(i));
            if(pred->is_hash_table()) {
               mem::allocator<hash_table>().construct(get_table(i));
               get_table(i)->setup(pred->get_hashed_field(), pred->get_field_type(pred->get_hashed_field())->get_type());
            } else {
               mem::allocator<tuple_list>().construct(get_list(i));
            }
         }
      }

      ~linear_store(void)
      {
         for(size_t i(0); i < num; ++i) {
            vm::predicate *pred(prog->get_predicate(i));
            if(pred->is_hash_table())
               mem::allocator<hash_table>().destroy(get_table(i));
            else
               mem::allocator<tuple_list>().destroy(get_list(i));
         }
         mem::allocator<hash_table>().deallocate(tables, num);
         mem::allocator<tuple_list>().deallocate(lists, num);
      }
};

}

#endif

