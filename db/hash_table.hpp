
#ifndef DB_HASH_TABLE_HPP
#define DB_HASH_TABLE_HPP

#include <assert.h>
#include <ostream>
#include <iostream>

#include "conf.hpp"
#include "vm/tuple.hpp"
#include "db/intrusive_list.hpp"
#include "mem/allocator.hpp"

namespace db
{

#define HASH_TABLE_INITIAL_TABLE_SIZE 8

typedef db::intrusive_list<vm::tuple> table_list;

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

            inline db::intrusive_list<vm::tuple>* operator*(void) const
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

      size_t insert(vm::tuple *);

      inline db::intrusive_list<vm::tuple>* lookup_list(const vm::tuple_field field)
      {
         const vm::uint_val id(hash_field(field));
         table_list *bucket(table + (id % size_table));
         return bucket;
      }

      inline void dump(std::ostream& out) const
      {
         for(size_t i(0); i < size_table; ++i) {
            table_list *ls(table + i);
            out << "Bucket for " << i << ": ";
            if(ls->empty())
               out << "empty\n";
            else
               out << "has " << ls->get_size() << " elements\n";
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
            if(total > size_table)
               return false;
         }
         return true;
      }

      inline void setup(const vm::field_num field, const vm::field_type type)
      {
         hash_argument = field;
         hash_type = type;
         size_table = HASH_TABLE_INITIAL_TABLE_SIZE;
         next_expand = NULL;
         table = alloc().allocate(HASH_TABLE_INITIAL_TABLE_SIZE);
         for(size_t i(0); i < size_table; ++i) {
            table_list *ls(table + i);
            alloc().construct(ls);
         }
      }

      ~hash_table(void)
      {
         for(size_t i(0); i < size_table; ++i) {
            table_list *ls(table + i);
            alloc().destroy(ls);
         }
         alloc().deallocate(table, size_table);
      }
};

}

#endif
