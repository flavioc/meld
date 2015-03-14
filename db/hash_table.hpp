
#ifndef DB_HASH_TABLE_HPP
#define DB_HASH_TABLE_HPP

#include <assert.h>
#include <ostream>
#include <iostream>

#include "vm/tuple.hpp"
#include "utils/intrusive_list.hpp"
#include "mem/allocator.hpp"
#include "utils/utils.hpp"

namespace db {

#define HASH_TABLE_INITIAL_TABLE_SIZE 16

struct hash_table {
   private:
   using tuple_list = vm::tuple_list;
   using allocator = mem::allocator<tuple_list>;

   tuple_list *table;
   std::uint16_t prime;
   std::uint16_t size_table;
   vm::field_type hash_type;

   inline uint64_t mod_hash(const uint64_t hsh) {
      return hsh % prime;
   }

   inline vm::uint_val hash_field(const vm::tuple_field field) const {
      switch (hash_type) {
         case vm::FIELD_INT:
            return utils::fnv1_hash((utils::byte*)&FIELD_INT(field), sizeof(vm::int_val));
         case vm::FIELD_FLOAT:
            return utils::fnv1_hash((utils::byte*)&FIELD_FLOAT(field), sizeof(vm::float_val));
         case vm::FIELD_NODE:
#ifdef USE_REAL_NODES
            {
               // UGLY HACK to make this function inline
               utils::byte *data((utils::byte*)FIELD_PTR(field));
               data = data + sizeof(std::atomic<vm::ref_count>);
               return (vm::uint_val)*(vm::node_val*)data;
//               return utils::fnv1_hash((utils::byte*)&ret, sizeof(vm::uint_val));
            }
#else
            return utils::fnv1_hash((utils::byte*)&FIELD_NODE(field), sizeof(vm::node_val));
#endif
         case vm::FIELD_LIST:
            if (FIELD_PTR(field) == 0)
               return 0;
            else
               return 1;
         default:
            assert(false);
            return 0;
      }
   }

   inline vm::uint_val hash_tuple(vm::tuple *tpl, const vm::predicate* pred) const {
      return hash_field(tpl->get_field(pred->get_hashed_field()));
   }

   void change_table(const std::uint16_t, const vm::predicate*);

   public:

   inline size_t get_num_buckets(void) const { return size_table; }

   class iterator {
  private:
      tuple_list *bucket;
      tuple_list *finish;

      inline void find_good_bucket(void) {
         for (; bucket != finish; bucket++) {
            if (!bucket->empty()) return;
         }
      }

  public:
      inline bool end(void) const { return bucket == finish; }

      inline utils::intrusive_list<vm::tuple> *operator*(void) const {
         return bucket;
      }

      inline iterator &operator++(void) {
         bucket++;
         find_good_bucket();
         return *this;
      }

      inline iterator operator++(int) {
         bucket++;
         find_good_bucket();
         return *this;
      }

      explicit iterator(tuple_list *start_bucket, tuple_list *end_bucket)
          : bucket(start_bucket), finish(end_bucket) {
         find_good_bucket();
      }
   };

   inline iterator begin(void) { return iterator(table, table + size_table); }
   inline iterator begin(void) const {
      return iterator(table, table + size_table);
   }

   inline size_t get_table_size() const { return size_table; }

   inline size_t get_total_size(void) const {
      size_t total(0);
      for (size_t i(0); i < size_table; ++i) {
         tuple_list *ls(table + i);
         total += ls->get_size();
      }
      return total;
   }

   inline bool empty(void) const {
      for (size_t i(0); i < size_table; ++i) {
         tuple_list *ls(table + i);
         if (!ls->empty()) return false;
      }
      return true;
   }

   size_t insert(vm::tuple *item, const vm::predicate *pred) {
      const vm::uint_val id(hash_tuple(item, pred));
      tuple_list *bucket(table + mod_hash(id));
      bucket->push_back(item);
      return bucket->get_size();
   }

   size_t insert_front(vm::tuple *, const vm::predicate*);

   inline utils::intrusive_list<vm::tuple> *lookup_list(
       const vm::tuple_field field) {
      const vm::uint_val id(hash_field(field));
      tuple_list *bucket(table + mod_hash(id));
      return bucket;
   }

   inline void dump(std::ostream &out, const vm::predicate *pred) const {
      for (size_t i(0); i < size_table; ++i) {
         tuple_list *ls(table + i);
         out << "Bucket for " << i << ": ";
         if (ls->empty())
            out << "empty\n";
         else {
            out << "has " << ls->get_size() << " elements:\n";
            ls->dump(out, pred);
         }
      }
   }

   inline void expand(const vm::predicate *pred) {
      const size_t new_size(size_table * 2);
      std::cout << "expand " << new_size << "\n";
      change_table(new_size, pred);
   }
   inline void shrink(const vm::predicate *pred) {
      const size_t new_size(size_table / 2);
//      std::cout << "shrink " << new_size << "\n";
      change_table(new_size, pred);
   }

   inline bool too_crowded(void) const {
      size_t crowded_buckets(0);
      for (size_t i(0); i < size_table; ++i) {
         tuple_list *ls(table + i);
         crowded_buckets += (ls->get_size() / 3);
      }
      if (crowded_buckets == 0) return false;
      return crowded_buckets > size_table / 2;
   }

   inline bool smallest_possible(void) const {
      return size_table == HASH_TABLE_INITIAL_TABLE_SIZE;
   }

   inline bool too_sparse(void) const {
      size_t total(0);
      for (size_t i(0); i < size_table; ++i) {
         tuple_list *ls(table + i);
         total += ls->get_size();
         if (total >= size_table) return false;
      }
      return true;
   }

   inline void setup(const vm::field_type type,
       const size_t default_table_size = HASH_TABLE_INITIAL_TABLE_SIZE) {
      hash_type = type;
      size_table = default_table_size;
      prime = utils::previous_prime(size_table);
      table = allocator().allocate(size_table);
      memset(table, 0, sizeof(tuple_list) * size_table);
   }

   inline void destroy(void) { allocator().deallocate(table, size_table); }
};
}

#endif
