
#ifndef DB_LINEAR_STORE_HPP
#define DB_LINEAR_STORE_HPP

#include <list>
#include <set>
#include <iostream>
#include <unordered_map>

#include "mem/base.hpp"
#include "db/tuple.hpp"
#include "vm/defs.hpp"
#include "vm/bitmap.hpp"
#include "db/intrusive_list.hpp"
#include "db/hash_table.hpp"
#include "utils/spinlock.hpp"

#define CREATE_HASHTABLE_THREADSHOLD 8

namespace db
{

struct linear_store
{
   public:

      typedef intrusive_list<vm::tuple> tuple_list;
      typedef std::unordered_map<vm::predicate_id, void*, std::hash<vm::predicate_id>,
              std::equal_to<vm::predicate_id>, mem::allocator< std::pair<const vm::predicate_id, void*> > > container_map;
      container_map structs;

      vm::bitmap types;
      utils::spinlock internal;

      hash_table *expand;

   private:

      inline hash_table* get_table(const vm::predicate_id p)
      {
         container_map::iterator it(structs.find(p));
         if(it == structs.end())
            return NULL;
         return (hash_table*)it->second;
      }

      inline hash_table* get_table(const vm::predicate_id p) const
      {
         container_map::const_iterator it(structs.find(p));
         if(it == structs.end())
            return NULL;
         return (hash_table*)it->second;
      }

      inline hash_table* create_table(const vm::predicate *pred)
      {
         hash_table *table(mem::allocator<hash_table>().allocate(1));
         mem::allocator<hash_table>().construct(table);
         structs[pred->get_id()] = (void*)table;
         table->setup(pred->get_hashed_field(), pred->get_field_type(pred->get_hashed_field())->get_type());
         return table;
      }

      inline tuple_list* create_list(const vm::predicate *pred)
      {
         tuple_list *add(mem::allocator<tuple_list>().allocate(1));
         mem::allocator<tuple_list>().construct(add);
         structs[pred->get_id()] = (void*)add;
         return add;
      }

      inline tuple_list* get_list(const vm::predicate_id p)
      {
         container_map::iterator it(structs.find(p));
         if(it == structs.end())
            return NULL;
         return (tuple_list*)it->second;
      }

      inline const tuple_list* get_list(const vm::predicate_id p) const
      {
         container_map::const_iterator it(structs.find(p));
         if(it == structs.end())
            return NULL;
         return (tuple_list*)it->second;
      }

      inline hash_table *transform_list_to_hash_table(table_list *ls, const vm::predicate *pred)
      {
         hash_table *tbl(create_table(pred));

         for(tuple_list::iterator it(ls->begin()), end(ls->end()); it != end; ) {
            vm::tuple *tpl(*it);
            ++it;
            tbl->insert(tpl);
         }

         mem::allocator<tuple_list>().destroy(ls);
         mem::allocator<tuple_list>().deallocate(ls, 1);

         types.set_bit(pred->get_id());

         return tbl;
      }

      inline tuple_list* transform_hash_table_to_list(hash_table *tbl, const vm::predicate *pred)
      {
         tuple_list *ls(create_list(pred));

         for(hash_table::iterator it(tbl->begin()); !it.end(); ) {
            tuple_list *bucket_ls(*it);
            ++it;

            ls->splice_back(*bucket_ls);
         }

         mem::allocator<hash_table>().destroy(tbl);
         mem::allocator<hash_table>().deallocate(tbl, 1);

         types.unset_bit(pred->get_id());

         return ls;
      }

      inline bool decide_if_expand(vm::rule_matcher& m, const vm::predicate *pred, hash_table *table) const
      {
         return m.get_count(pred->get_id()) >= table->get_num_buckets();
      }

   public:

      inline tuple_list* get_linked_list(const vm::predicate_id p) { return get_list(p); }
      inline const tuple_list* get_linked_list(const vm::predicate_id p) const { return get_list(p); }

      inline hash_table* get_hash_table(const vm::predicate_id p) { return get_table(p); }
      inline const hash_table* get_hash_table(const vm::predicate_id p) const { return get_table(p); }

      inline bool stored_as_hash_table(const vm::predicate *pred) const { return types.get_bit(pred->get_id()); }

      inline void add_fact(vm::tuple *tpl, vm::predicate *pred, vm::rule_matcher& m)
      {
         if(pred->is_hash_table()) {
            if(stored_as_hash_table(pred)) {
               hash_table *table(get_table(pred->get_id()));
               if(table == NULL)
                  table = create_table(pred);
               size_t size_bucket(table->insert(tpl));
               if(!table->next_expand && size_bucket >= CREATE_HASHTABLE_THREADSHOLD) {
                  if(decide_if_expand(m, pred, table)) {
                     assert(!table->next_expand);
                     table->next_expand = expand;
                     expand = table;
                  }
               }
            } else {
               // still using a list
               table_list *ls(get_list(pred->get_id()));
               if(ls == NULL)
                  ls = create_list(pred);

               if(ls->get_size() + 1 >= CREATE_HASHTABLE_THREADSHOLD) {
                  hash_table *table(transform_list_to_hash_table(ls, pred));
                  table->insert(tpl);
               } else
                  ls->push_back(tpl);
            }
         } else {
            table_list *ls(get_list(pred->get_id()));
            if(ls == NULL)
               ls = create_list(pred);
            ls->push_back(tpl);
         }
      }

      inline void increment_database(vm::predicate *pred, tuple_list *ls, vm::rule_matcher& m)
      {
         if(pred->is_hash_table()) {
            hash_table *table(get_table(pred->get_id()));
            if(table == NULL)
               table = create_table(pred);
            bool big_bucket(false);
            for(tuple_list::iterator it(ls->begin()), end(ls->end()); it != end; ++it) {
               const size_t size_bucket(table->insert(*it));
               if(size_bucket >= CREATE_HASHTABLE_THREADSHOLD)
                  big_bucket = true;
            }
            ls->clear();
            if(!table->next_expand && big_bucket) {
               if(decide_if_expand(m, pred, table)) {
                  assert(!table->next_expand);
                  table->next_expand = expand;
                  expand = table;
               }
            }
         } else {
            table_list *add(get_list(pred->get_id()));
            if(add == NULL)
               add = create_list(pred);
            add->splice_back(*ls);
         }
      }

      inline void improve_index(void)
      {
         for(hash_table *tbl(expand); tbl != NULL; ) {
            hash_table *next(tbl->next_expand);
            tbl->next_expand = NULL;
            if(tbl->too_crowded())
               tbl->expand();
            tbl = next;
         }
      }

      inline void cleanup_index(void)
      {
         for(vm::bitmap::iterator it(types.begin(vm::theProgram->num_predicates())); !it.end(); ++it) {
            const vm::predicate_id id(*it);
            hash_table *tbl(get_table(id));

            if(tbl) {
               if(tbl->too_sparse()) {
                  if(tbl->smallest_possible())
                     transform_hash_table_to_list(tbl, vm::theProgram->get_predicate(id));
                  else
                     tbl->shrink();
               }
            }
         }
      }

      explicit linear_store(void)
      {
         vm::bitmap::create(types, vm::theProgram->num_predicates_next_uint());
         types.clear(vm::theProgram->num_predicates_next_uint());
         expand = NULL;
      }

      ~linear_store(void)
      {
         vm::bitmap::destroy(types, vm::theProgram->num_predicates_next_uint());
         for(container_map::iterator it(structs.begin()), end(structs.end()); it != end; ++it) {
            if(types.get_bit(it->first)) {
               hash_table *table((hash_table*)it->second);
               mem::allocator<hash_table>().destroy(table);
               mem::allocator<hash_table>().deallocate(table, 1);
            } else {
               tuple_list *list((tuple_list*)it->second);
               mem::allocator<tuple_list>().destroy(list);
               mem::allocator<tuple_list>().deallocate(list, 1);
            }
         }
      }
};

}

#endif

