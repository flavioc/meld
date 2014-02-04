
#ifndef DB_HASH_TABLE_HPP
#define DB_HASH_TABLE_HPP

#include <assert.h>
#include <ostream>
#include <iostream>

#include "conf.hpp"
#include "vm/tuple.hpp"
#include "db/intrusive_list.hpp"
#include "mem/allocator.hpp"

#define MIXED_BUCKET 1

namespace db
{

#define HASH_TABLE_INITIAL_TABLE_SIZE 8

#ifndef MIXED_BUCKET
typedef struct _table_node {
   struct _table_node *next;
   vm::tuple_field field;
   db::intrusive_list<vm::tuple> ls;
} table_node;

struct table_list {
   table_node *head;
   size_t size;
};
#else
typedef db::intrusive_list<vm::tuple> table_list;
#endif

struct hash_table
{
   private:

      typedef mem::allocator<table_list> alloc;

      table_list *table;
      size_t size_table;
      vm::field_num hash_argument;
      vm::field_type hash_type;
      size_t added_so_far;
      size_t check_limit;

      vm::uint_val hash_field(const vm::tuple_field field) const;

      inline vm::uint_val hash_tuple(vm::tuple *tpl) const
      {
         return hash_field(tpl->get_field(hash_argument));
      }

   public:

      class iterator
      {
         private:

            table_list *bucket;
            table_list *finish;
#ifndef MIXED_BUCKET
            table_node *cur;
#endif

            inline void find_good_bucket(void)
            {
               for(; bucket != finish; bucket++) {
#ifdef MIXED_BUCKET
                  if(!bucket->empty())
                     return;
#else
                  if(bucket->head) {
                     cur = bucket->head;
                     return;
                  }
#endif
               }
            }

         public:

            inline bool end(void) const { return bucket == finish; }

            inline db::intrusive_list<vm::tuple>* operator*(void) const
            {
#ifdef MIXED_BUCKET
               return bucket;
#else
               return &(cur->ls);
#endif
            }

            inline iterator& operator++(void)
            {
#ifdef MIXED_BUCKET
               bucket++;
               find_good_bucket();
#else
               cur = cur->next;
               if(!cur) {
                  bucket++;
                  find_good_bucket();
               }
#endif
               return *this;
            }

            inline iterator operator++(int)
            {
#ifdef MIXED_BUCKET
               bucket++;
               find_good_bucket();
#else
               cur = cur->next;
               if(!cur) {
                  bucket++;
                  find_good_bucket();
               }
#endif
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

      void insert(vm::tuple *);

      inline db::intrusive_list<vm::tuple>* lookup_list(const vm::tuple_field field)
      {
         const vm::uint_val id(hash_field(field));
         table_list *bucket(table + (id % size_table));
#ifdef MIXED_BUCKET
         return bucket;
#else
         table_node *node(bucket->head);
         for(; node != NULL; node = node->next) {
            const vm::tuple_field other(node->field);
            switch(hash_type) {
               case vm::FIELD_INT:
                  if(FIELD_INT(field) == FIELD_INT(other))
                     return &(node->ls);
                  break;
               case vm::FIELD_FLOAT:
                  if(FIELD_FLOAT(field) == FIELD_FLOAT(other))
                     return &(node->ls);
                  break;
               case vm::FIELD_NODE:
                  if(FIELD_NODE(field) == FIELD_NODE(other))
                     return &(node->ls);
                  break;
               default: assert(false); break;
            }
         }
         return NULL;
#endif
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

      inline void reset_added(void)
      {
         added_so_far = 0;
      }

      inline bool must_check_hash_table(void) const
      {
         return added_so_far > check_limit;
      }

      void expand(void);

      inline bool too_crowded(void) const
      {
         size_t crowded_buckets(0);
         for(size_t i(0); i < size_table; ++i) {
            table_list *ls(table + i);
            crowded_buckets += (ls->get_size() / 4);
         }
         if(crowded_buckets == 0)
            return false;
         return crowded_buckets > size_table/2;
      }

      inline void setup(const vm::field_num field, const vm::field_type type)
      {
         hash_argument = field;
         hash_type = type;
         size_table = HASH_TABLE_INITIAL_TABLE_SIZE;
         added_so_far = 0;
         check_limit = 64;
         table = alloc().allocate(HASH_TABLE_INITIAL_TABLE_SIZE);
         for(size_t i(0); i < size_table; ++i) {
            table_list *ls(table + i);
#ifdef MIXED_BUCKET
            alloc().construct(ls);
#else
            ls->head = NULL;
            ls->size = 0;
#endif
         }
      }

      ~hash_table(void)
      {
         for(size_t i(0); i < size_table; ++i) {
            table_list *ls(table + i);
#ifdef MIXED_BUCKET
            alloc().destroy(ls);
#else
            for(table_node *node(ls->head); node != NULL; ) {
               table_node *next(node->next);

               mem::allocator<table_node>().destroy(node);
               mem::allocator<table_node>().deallocate(node, 1);

               node = next;
            }
#endif
         }
         alloc().deallocate(table, size_table);
      }
};

}

#endif
