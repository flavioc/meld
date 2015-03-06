
#ifndef DB_LINEAR_STORE_HPP
#define DB_LINEAR_STORE_HPP

#include <list>
#include <set>
#include <iostream>
#include <unordered_map>

#include "utils/mutex.hpp"
#include "mem/base.hpp"
#include "vm/defs.hpp"
#include "vm/all.hpp"
#include "vm/bitmap.hpp"
#include "utils/intrusive_list.hpp"
#include "db/hash_table.hpp"
#ifdef COMPILED
#include COMPILED_HEADER
#endif

#define CREATE_HASHTABLE_THREADSHOLD 8
#define ITEM_SIZE ((sizeof(hash_table) > sizeof(tuple_list) ? sizeof(hash_table) : sizeof(tuple_list)))

namespace db
{

struct linear_store
{
   public:

      using tuple_list = vm::tuple_list;

      // we store N lists or hash tables (the biggest structure) so that it is contiguous
#ifdef COMPILED
      utils::byte data[ITEM_SIZE * COMPILED_NUM_LINEAR];
#else
      utils::byte *data;
#endif

      vm::bitmap types;

      hash_table *expand;

   private:

      inline hash_table* get_table(const vm::predicate_id p)
      {
         assert(p < vm::theProgram->num_linear_predicates());
         return (hash_table*)(data + ITEM_SIZE * p);
      }

      inline hash_table* get_table(const vm::predicate_id p) const
      {
         assert(p < vm::theProgram->num_linear_predicates());
         return (hash_table*)(data + ITEM_SIZE * p);
      }

      inline hash_table* create_table(const vm::predicate *pred)
      {
         hash_table *table(get_table(pred->get_linear_id()));
         mem::allocator<hash_table>().construct(table);
         const vm::field_num field(pred->get_hashed_field());
         table->setup(field, pred->get_field_type(field)->get_type());
         return table;
      }

      inline tuple_list* get_list(const vm::predicate_id p)
      {
         assert(p < vm::theProgram->num_linear_predicates());
         return (tuple_list*)(data + ITEM_SIZE * p);
      }

      inline const tuple_list* get_list(const vm::predicate_id p) const
      {
         assert(p < vm::theProgram->num_linear_predicates());
         return (tuple_list*)(data + ITEM_SIZE * p);
      }

      inline hash_table *transform_hash_table_new_field(hash_table *tbl, const vm::predicate *pred)
      {
         const size_t size_table(tbl->get_num_buckets());
         hash_table new_hash;
         const vm::field_num field(pred->get_hashed_field());
         new_hash.setup(field, pred->get_field_type(field)->get_type(), size_table);

         hash_table::iterator it(tbl->begin());

         while(!it.end()) {
            tuple_list *bucket_ls(*it);
            ++it;

            for(auto tpl : *bucket_ls) {
               

               new_hash.insert(tpl);
            }
         }

         tbl->destroy();
         // copy new
         memcpy(tbl, &new_hash, sizeof(hash_table));

         return tbl;
      }

      inline hash_table *transform_list_to_hash_table(tuple_list *ls, const vm::predicate *pred)
      {
         tuple_list::iterator it(ls->begin());
         tuple_list::iterator end(ls->end());
         hash_table *tbl(create_table(pred));

         while(it != end) {
            vm::tuple *tpl(*it);
            ++it;
            tbl->insert(tpl);
         }

         types.set_bit(pred->get_linear_id());

         return tbl;
      }

      inline tuple_list* transform_hash_table_to_list(hash_table *tbl, const vm::predicate *pred)
      {
         hash_table::iterator it(tbl->begin());
         assert(tbl->next_expand == nullptr);
         tuple_list ls;
         // cannot get list immediatelly since that would remove the pointer to the hash table data

         while(!it.end()) {
            tuple_list *bucket_ls(*it);
            ++it;

            ls.splice_back(*bucket_ls);
         }

         tbl->destroy();
         memcpy(tbl, &ls, sizeof(tuple_list));

         types.unset_bit(pred->get_linear_id());

         return (tuple_list*)tbl;
      }

      inline bool decide_if_expand(hash_table *table) const
      {
         return table->get_total_size() >= table->get_num_buckets();
      }

   public:

      inline bool empty(const vm::predicate_id id) const {
         return get_list(id)->empty();
      }

      inline tuple_list* get_linked_list(const vm::predicate_id p) { return get_list(p); }
      inline const tuple_list* get_linked_list(const vm::predicate_id p) const { return get_list(p); }

      inline hash_table* get_hash_table(const vm::predicate_id p) { return get_table(p); }
      inline const hash_table* get_hash_table(const vm::predicate_id p) const { return get_table(p); }

      inline bool stored_as_hash_table(const vm::predicate *pred) const { return types.get_bit(pred->get_linear_id()); }

      inline void add_fact_list(vm::tuple *tpl, const vm::predicate_id id)
      {
         tuple_list *ls(get_list(id));
         ls->push_back(tpl);
      }

      inline void add_fact(vm::tuple *tpl, vm::predicate *pred)
      {
         if(pred->is_hash_table()) {
            if(stored_as_hash_table(pred)) {
               hash_table *table(get_table(pred->get_linear_id()));
               size_t size_bucket(table->insert(tpl));
               if(!table->next_expand && size_bucket >= CREATE_HASHTABLE_THREADSHOLD) {
                  if(decide_if_expand(table)) {
                     assert(!table->next_expand);
                     table->next_expand = expand;
                     expand = table;
                  }
               }
            } else {
               // still using a list
               tuple_list *ls(get_list(pred->get_linear_id()));

               if(ls->get_size() + 1 >= CREATE_HASHTABLE_THREADSHOLD) {
                  hash_table *table(transform_list_to_hash_table(ls, pred));
                  table->insert(tpl);
               } else
                  ls->push_back(tpl);
            }
         } else {
            if(stored_as_hash_table(pred)) {
               hash_table *table(get_table(pred->get_linear_id()));
               table->insert(tpl);
            } else
               add_fact_list(tpl, pred->get_linear_id());
         }
      }

      inline void increment_database(vm::predicate *pred, tuple_list *ls)
      {
         if(pred->is_hash_table()) {
            if(stored_as_hash_table(pred)) {
               hash_table *table(get_table(pred->get_linear_id()));
               bool big_bucket(false);
               for(tuple_list::iterator it(ls->begin()), end(ls->end()); it != end;) {
                  vm::tuple *tpl(*it);
                  it++;
                  const size_t size_bucket(table->insert(tpl));
                  if(size_bucket >= CREATE_HASHTABLE_THREADSHOLD)
                     big_bucket = true;
               }
               ls->clear();
               if(!table->next_expand && big_bucket) {
                  if(decide_if_expand(table)) {
                     assert(!table->next_expand);
                     table->next_expand = expand;
                     expand = table;
                  }
               }
            } else {
               tuple_list *add(get_list(pred->get_linear_id()));

               if(add->get_size() + ls->get_size() >= CREATE_HASHTABLE_THREADSHOLD) {
                  hash_table *table(transform_list_to_hash_table(add, pred));
                  for(tuple_list::iterator it(ls->begin()), end(ls->end()); it != end; ) {
                     vm::tuple *tpl(*it);
                     it++;
                     table->insert(tpl);
                  }
                  ls->clear();
               } else
                  add->splice_back(*ls);
            }
         } else {
            if(stored_as_hash_table(pred)) {
               hash_table *table(get_table(pred->get_linear_id()));
               for(tuple_list::iterator it(ls->begin()), end(ls->end()); it != end;) {
                  vm::tuple *tpl(*it);
                  it++;
                  table->insert(tpl);
               }
               ls->clear();
            } else {
               tuple_list *add(get_list(pred->get_linear_id()));
               add->splice_back(*ls);
            }
         }
      }

      inline void improve_index(void)
      {
         while(expand) {
            hash_table *next(expand->next_expand);
            expand->next_expand = nullptr;
            if(expand->too_crowded())
               expand->expand();
            expand = next;
         }
      }

      inline void cleanup_index(void)
      {
         for(vm::bitmap::iterator it(types.begin(vm::theProgram->num_predicates())); !it.end(); ++it) {
            const vm::predicate_id id(*it);
            vm::predicate *pred(vm::theProgram->get_predicate(id));
            if(!pred->is_linear_pred())
               continue;

            hash_table *tbl(get_table(pred->get_linear_id()));

            if(tbl->too_sparse()) {
               if(tbl->smallest_possible())
                  transform_hash_table_to_list(tbl, pred);
               else
                  tbl->shrink();
            }
         }
      }

      inline void rebuild_index(void)
      {
         for(size_t i(0); i < vm::theProgram->num_predicates(); ++i) {
            vm::predicate *pred(vm::theProgram->get_predicate(i));

            if(!pred->is_linear_pred())
               continue;

            if(pred->is_hash_table()) {
               if(stored_as_hash_table(pred)) {
                  hash_table *tbl(get_hash_table(pred->get_linear_id()));
                  // check if using the correct argument
                  const vm::field_num old(tbl->get_hash_argument());
                  if(old != pred->get_hashed_field()) {
                     transform_hash_table_new_field(tbl, pred);
                  } else {
                     // do nothing
                  }
               } else {
                  tuple_list *ls(get_linked_list(pred->get_linear_id()));
                  if(ls->get_size() >= CREATE_HASHTABLE_THREADSHOLD)
                     transform_list_to_hash_table(ls, pred);
               }
            } else {
               if(stored_as_hash_table(pred)) {
                  hash_table *tbl(get_hash_table(pred->get_linear_id()));
                  transform_hash_table_to_list(tbl, pred);
               } else {
                  // do nothing...
               }
            }
         }
      }

      explicit linear_store(void)
      {
         vm::bitmap::create(types, vm::theProgram->num_linear_predicates_next_uint());
         types.clear(vm::theProgram->num_linear_predicates_next_uint());
#ifndef COMPILED
         data = mem::allocator<utils::byte>().allocate(ITEM_SIZE * vm::theProgram->num_linear_predicates());
#endif
         for(size_t i(0); i < vm::theProgram->num_linear_predicates(); ++i) {
            utils::byte *p(data + ITEM_SIZE * i);
            mem::allocator<tuple_list>().construct((tuple_list*)p);
         }
         expand = nullptr;
      }

      inline void destroy(vm::candidate_gc_nodes& gc_nodes) {
         for(size_t i(0); i < vm::theProgram->num_predicates(); ++i) {
            vm::predicate *pred(vm::theProgram->get_predicate(i));
            if(!pred->is_linear_pred())
               continue;
            utils::byte *p(data + ITEM_SIZE * pred->get_linear_id());
            if(types.get_bit(i)) {
               hash_table *table((hash_table*)p);
               for(hash_table::iterator it(table->begin()); !it.end(); ++it) {
                  tuple_list *ls(*it);
                  for(tuple_list::iterator it2(ls->begin()), end(ls->end()); it2 != end; ) {
                     vm::tuple *tpl(*it2);
                     it2++;
                     vm::tuple::destroy(tpl, pred, gc_nodes);
                  }
               }
               // turn into list
               table->destroy();
               mem::allocator<tuple_list>().construct((tuple_list*)table);
               types.unset_bit(i);
            } else {
               tuple_list *ls((tuple_list*)p);
               for(tuple_list::iterator it(ls->begin()), end(ls->end()); it != end; ) {
                  vm::tuple *tpl(*it);
                  ++it;
                  vm::tuple::destroy(tpl, pred, gc_nodes);
               }
               ls->clear();
            }
         }
      }

      ~linear_store(void)
      {
         for(size_t i(0); i < vm::theProgram->num_linear_predicates(); ++i) {
            utils::byte *p(data + ITEM_SIZE * i);
            if(types.get_bit(i)) {
               hash_table *table((hash_table*)p);
               table->destroy();
            }
            // nothing to destroy at the list level
         }
#ifndef COMPILED
         mem::allocator<utils::byte>().deallocate(data, ITEM_SIZE * vm::theProgram->num_linear_predicates());
#endif
         vm::bitmap::destroy(types, vm::theProgram->num_linear_predicates_next_uint());
      }
};

}

#endif
