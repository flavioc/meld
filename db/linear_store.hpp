
#ifndef DB_LINEAR_STORE_HPP
#define DB_LINEAR_STORE_HPP

#include <list>
#include <set>
#include <iostream>
#include <unordered_map>

#include "mem/base.hpp"
#include "db/tuple.hpp"
#include "vm/defs.hpp"
#include "db/intrusive_list.hpp"
#include "db/hash_table.hpp"
#include "utils/spinlock.hpp"

#define EXPAND_HASH_TABLES 1
#define USE_UNORDERED_MAP 1

namespace db
{

struct linear_store
{
   public:

      typedef intrusive_list<vm::tuple> tuple_list;
#ifdef USE_UNORDERED_MAP
      typedef std::unordered_map<vm::predicate_id, tuple_list*, std::hash<vm::predicate_id>,
              std::equal_to<vm::predicate_id>, mem::allocator< std::pair<const vm::predicate_id, tuple_list*> > > list_map;
      list_map lists;
#else
      tuple_list *lists;
#endif

#ifdef USE_UNORDERED_MAP
      typedef std::unordered_map<vm::predicate_id, hash_table*, std::hash<vm::predicate_id>,
              std::equal_to<vm::predicate_id>, mem::allocator< std::pair<const vm::predicate_id, hash_table*> > > table_map;
      table_map tables;
#else
      hash_table *tables;
      vm::program *prog;
#endif

      utils::spinlock internal;

#ifdef EXPAND_HASH_TABLES
      typedef std::set<hash_table*, std::less<hash_table*>, mem::allocator<hash_table*> > set_tables_expand;
      set_tables_expand tables_to_expand;
#endif

   private:

      inline hash_table* get_table(const vm::predicate_id p)
      {
#ifdef USE_UNORDERED_MAP
         table_map::iterator it(tables.find(p));
         if(it == tables.end())
            return NULL;
         return it->second;
#else
         return tables + p;
#endif
      }

      inline hash_table* get_table(const vm::predicate_id p) const
      {
#ifdef USE_UNORDERED_MAP
         table_map::const_iterator it(tables.find(p));
         if(it == tables.end())
            return NULL;
         return it->second;
#else
         return tables + p;
#endif
      }

      inline hash_table* create_table(const vm::predicate *pred)
      {
         hash_table *table(mem::allocator<hash_table>().allocate(1));
         mem::allocator<hash_table>().construct(table);
         tables.insert(std::make_pair(pred->get_id(), table));
         table->setup(pred->get_hashed_field(), pred->get_field_type(pred->get_hashed_field())->get_type());
         return table;
      }

      inline tuple_list* get_list(const vm::predicate_id p)
      {
#ifdef USE_UNORDERED_MAP
         list_map::iterator it(lists.find(p));
         if(it == lists.end())
            return NULL;
         return it->second;
#else
         return lists + p;
#endif
      }

      inline const tuple_list* get_list(const vm::predicate_id p) const
      {
#ifdef USE_UNORDERED_MAP
         list_map::const_iterator it(lists.find(p));
         if(it == lists.end())
            return NULL;
         return it->second;
#else
         return lists + p;
#endif
      }

   public:

      inline tuple_list* get_linked_list(const vm::predicate_id p) { return get_list(p); }
      inline const tuple_list* get_linked_list(const vm::predicate_id p) const { return get_list(p); }

      inline hash_table* get_hash_table(const vm::predicate_id p) { return get_table(p); }
      inline const hash_table* get_hash_table(const vm::predicate_id p) const { return get_table(p); }

      inline void add_fact(vm::tuple *tpl)
      {
         const vm::predicate *pred(tpl->get_predicate());
         if(pred->is_hash_table()) {
            hash_table *table(get_table(pred->get_id()));
#ifdef USE_UNORDERED_MAP
            if(table == NULL)
               table = create_table(pred);
#endif
            table->insert(tpl);
#ifdef EXPAND_HASH_TABLES
            if(table->must_check_hash_table())
               tables_to_expand.insert(table);
#endif
         } else {
            table_list *ls(get_list(pred->get_id()));
#ifdef USE_UNORDERED_MAP
            if(ls == NULL) {
               ls = mem::allocator<tuple_list>().allocate(1);
               mem::allocator<tuple_list>().construct(ls);
               lists.insert(std::make_pair(pred->get_id(), ls));
            }
#endif
            ls->push_back(tpl);
         }
      }

      inline void increment_database(vm::predicate *pred, tuple_list *ls)
      {
         if(pred->is_hash_table()) {
            hash_table *table(get_table(pred->get_id()));
#ifdef USE_UNORDERED_MAP
            if(table == NULL)
               table = create_table(pred);
#endif
            for(tuple_list::iterator it(ls->begin()), end(ls->end()); it != end; ++it)
               table->insert(*it);
            ls->clear();
#ifdef EXPAND_HASH_TABLES
            if(table->must_check_hash_table())
               tables_to_expand.insert(table);
#endif
         } else {
            table_list *add(get_list(pred->get_id()));
#ifdef USE_UNORDERED_MAP
            if(add == NULL) {
               add = mem::allocator<tuple_list>().allocate(1);
               mem::allocator<tuple_list>().construct(add);
               lists[pred->get_id()] = add;
            }
#endif
            add->splice_back(*ls);
         }
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
            }
            table->reset_added();
         }
         tables_to_expand.clear();
#endif
      }

      explicit linear_store(vm::program *_prog)
#ifndef USE_UNORDERED_MAP
         : prog(_prog)
#endif
      {
         (void)_prog;
#ifndef USE_UNORDERED_MAP
         lists = NULL;
         tables = NULL;
         for(size_t i(0); i < prog->num_predicates(); ++i) {
            vm::predicate *pred(prog->get_predicate(i));
            if(pred->is_hash_table()) {
               if(!tables)
                  tables = mem::allocator<hash_table>().allocate(prog->num_predicates());
               mem::allocator<hash_table>().construct(get_table(i));
               get_table(i)->setup(pred->get_hashed_field(), pred->get_field_type(pred->get_hashed_field())->get_type());
            } else {
               if(!lists)
                  lists = mem::allocator<tuple_list>().allocate(prog->num_predicates());
               mem::allocator<tuple_list>().construct(get_list(i));
            }
         }
#endif
      }

      ~linear_store(void)
      {
#ifdef USE_UNORDERED_MAP
         for(table_map::iterator it(tables.begin()), end(tables.end()); it != end; ++it) {
            hash_table *table(it->second);
            mem::allocator<hash_table>().destroy(table);
            mem::allocator<hash_table>().deallocate(table, 1);
         }
         for(list_map::iterator it(lists.begin()), end(lists.end()); it != end; ++it) {
            tuple_list *list(it->second);
            mem::allocator<tuple_list>().destroy(list);
            mem::allocator<tuple_list>().deallocate(list, 1);
         }
#else
         for(size_t i(0); i < prog->num_predicates(); ++i) {
            vm::predicate *pred(prog->get_predicate(i));
            if(pred->is_hash_table()) {
               assert(tables);
               mem::allocator<hash_table>().destroy(get_table(i));
            } else {
               assert(lists);
               mem::allocator<tuple_list>().destroy(get_list(i));
            }
         }
         if(tables)
            mem::allocator<hash_table>().deallocate(tables, prog->num_predicates());
         if(lists)
            mem::allocator<tuple_list>().deallocate(lists, prog->num_predicates());
#endif
      }
};

}

#endif

