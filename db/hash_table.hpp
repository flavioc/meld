
#ifndef DB_HASH_TABLE_HPP
#define DB_HASH_TABLE_HPP

#include <assert.h>
#include <ostream>
#include <iostream>

#include "vm/tuple.hpp"
#include "utils/intrusive_list.hpp"
#include "mem/allocator.hpp"

namespace db
{

#define HASH_TABLE_INITIAL_TABLE_SIZE 8

typedef utils::intrusive_list<vm::tuple> table_list;

struct hash_table
{
   private:

      typedef mem::allocator<table_list> alloc;

      table_list *table;
      size_t size_table;
      vm::field_num hash_argument;
      vm::field_type hash_type;

      vm::uint_val hash_field(const vm::tuple_field field) const;

      inline vm::uint_val hash_tuple(vm::tuple *tpl) const
      {
         return hash_field(tpl->get_field(hash_argument));
      }

      void change_table(const size_t);

   public:

      hash_table *next_expand;

      inline size_t get_num_buckets(void) const { return size_table; }
      inline vm::field_num get_hash_argument(void) const { return hash_argument; }

      class iterator
      {
         private:

            table_list *bucket;
            table_list *finish;

            inline void find_good_bucket(void)
            {
               for(; bucket != finish; bucket++) {
                  if(!bucket->empty())
                     return;
               }
            }

         public:

            inline bool end(void) const { return bucket == finish; }

            inline utils::intrusive_list<vm::tuple>* operator*(void) const
            {
               return bucket;
            }

            inline iterator& operator++(void)
            {
               bucket++;
               find_good_bucket();
               return *this;
            }

            inline iterator operator++(int)
            {
               bucket++;
               find_good_bucket();
               return *this;
            }

            explicit iterator(table_list *start_bucket, table_list *end_bucket):
               bucket(start_bucket), finish(end_bucket)
            {
               find_good_bucket();
            }
      };

      inline iterator begin(void) { return iterator(table, table + size_table); }
      inline iterator begin(void) const { return iterator(table, table + size_table); }

      inline size_t get_table_size() const { return size_table; }

      inline size_t get_total_size(void) const
      {
         size_t total(0);
         for(size_t i(0); i < size_table; ++i) {
            table_list *ls(table + i);
            total += ls->get_size();
         }
         return total;
      }

      size_t insert(vm::tuple *);
      size_t insert_front(vm::tuple *);

      inline utils::intrusive_list<vm::tuple>* lookup_list(const vm::tuple_field field)
      {
         const vm::uint_val id(hash_field(field));
         table_list *bucket(table + (id % size_table));
         return bucket;
      }

      inline void dump(std::ostream& out, const vm::predicate *pred) const
      {
         for(size_t i(0); i < size_table; ++i) {
            table_list *ls(table + i);
            out << "Bucket for " << i << ": ";
            if(ls->empty())
               out << "empty\n";
            else {
               out << "has " << ls->get_size() << " elements:\n";
               ls->dump(out, pred);
            }
         }
      }

      inline void expand(void) { change_table(size_table * 2); }
      inline void shrink(void) { change_table(size_table / 2); }

      inline bool too_crowded(void) const
      {
         size_t crowded_buckets(0);
         for(size_t i(0); i < size_table; ++i) {
            table_list *ls(table + i);
            crowded_buckets += (ls->get_size() / 3);
         }
         if(crowded_buckets == 0)
            return false;
         return crowded_buckets > size_table/2;
      }

      inline bool smallest_possible(void) const { return size_table == HASH_TABLE_INITIAL_TABLE_SIZE; }

      inline bool too_sparse(void) const
      {
         size_t total(0);
         for(size_t i(0); i < size_table; ++i) {
            table_list *ls(table + i);
            total += ls->get_size();
            if(total >= size_table)
               return false;
         }
         return true;
      }

      inline void setup(const vm::field_num field, const vm::field_type type,
            const size_t default_table_size = HASH_TABLE_INITIAL_TABLE_SIZE)
      {
         hash_argument = field;
         hash_type = type;
         next_expand = NULL;
         size_table = default_table_size;
         table = alloc().allocate(size_table);
         memset(table, 0, sizeof(table_list)*size_table);
      }

      inline void destroy(void)
      {
         alloc().deallocate(table, size_table);
      }
};

}

#endif
