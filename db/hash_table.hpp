
#ifndef DB_HASH_TABLE_HPP
#define DB_HASH_TABLE_HPP

#include <assert.h>
#include <ostream>
#include <iostream>

#include "vm/tuple.hpp"
#include "utils/intrusive_list.hpp"
#include "mem/allocator.hpp"
#include "utils/utils.hpp"
#include "utils/hash.hpp"

namespace db {

#define HASH_TABLE_INITIAL_TABLE_SIZE 8
#define CREATE_HASHTABLE_THREADSHOLD 4

struct hash_table_list {
   vm::tuple_list list;
   vm::tuple_field value;
   hash_table_list *prev{nullptr}, *next{nullptr};

   using iterator = vm::tuple_list::iterator;

   inline bool empty() const { return list.empty(); }

   inline size_t get_size() const { return list.get_size(); }

   inline void dump(std::ostream& out, const vm::predicate *pred) const { list.dump(out, pred); }

   inline void push_back(vm::tuple *item) { list.push_back(item); }
   inline void push_front(vm::tuple *item) { list.push_front(item); }

   inline vm::tuple_list::iterator begin() const { return list.begin(); }
   inline vm::tuple_list::iterator end() const { return list.end(); }

   inline iterator erase(iterator& it)
   {
      return list.erase(it);
   }
};

struct subhash_table {
   using tuple_list = hash_table_list;
   tuple_list **table;
   size_t unique_elems{0};
   std::uint16_t prime;
   std::uint16_t size_table;
   mutable utils::byte flag_expanded{10};

   inline uint64_t mod_hash(const uint64_t hsh) {
      return hsh % prime;
   }

   inline vm::uint_val hash_field(const vm::tuple_field field, const vm::field_type hash_type) const {
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
            abort();
            return 0;
      }
   }

   inline vm::uint_val hash_tuple(vm::tuple *tpl, const vm::predicate* pred, const vm::field_type hash_type) const {
      return hash_field(tpl->get_field(pred->get_hashed_field()), hash_type);
   }

   inline bool same_field0(const vm::tuple_field field1, const vm::tuple_field field2, const vm::field_type hash_type) const {
      switch (hash_type) {
         case vm::FIELD_INT:
            return FIELD_INT(field1) == FIELD_INT(field2);
         case vm::FIELD_FLOAT:
            return FIELD_FLOAT(field1) == FIELD_FLOAT(field2);
         case vm::FIELD_NODE:
            return FIELD_NODE(field1) == FIELD_NODE(field2);
         case vm::FIELD_LIST:
            return (FIELD_PTR(field1) && FIELD_PTR(field2)) || FIELD_PTR(field1) == FIELD_PTR(field2);
         default:
            abort();
            return 0;
      }
   }

   inline bool same_field(vm::tuple *tpl, const vm::tuple_field field, const vm::predicate *pred, const vm::field_type hash_type) const {
      return same_field0(tpl->get_field(pred->get_hashed_field()), field, hash_type);
   }

public:
   void change_table(const std::uint16_t, mem::node_allocator*, const vm::field_type);
   inline size_t get_table_size() const { return size_table; }

   size_t insert_front(vm::tuple *, const vm::predicate*, const vm::field_type);

   size_t insert(vm::tuple *item, const vm::predicate *pred, mem::node_allocator *alloc, const vm::field_type hash_type) {
      const vm::uint_val id(hash_tuple(item, pred, hash_type));
      const size_t idx(mod_hash(id));
      tuple_list *bucket(table[idx]);
      tuple_list *last{nullptr};
      size_t count{0};
      while(bucket) {
         if(same_field(item, bucket->value, pred, hash_type)) {
            bucket->push_back(item);
            return bucket->get_size();
         }
         last = bucket;
         bucket = bucket->next;
         count++;
      }

      tuple_list *newls((tuple_list*)alloc->allocate_obj(sizeof(tuple_list)));
      mem::allocator<tuple_list>().construct(newls);
      if(last) {
         newls->prev = last;
         last->next = newls;
      } else {
         newls->prev = nullptr;
         table[idx] = newls;
      }
      newls->value = item->get_field(pred->get_hashed_field());
      newls->next = nullptr;
      newls->push_back(item);
      unique_elems++;
      if(count + 1 >= CREATE_HASHTABLE_THREADSHOLD)
         expand(pred, alloc, hash_type);
      return newls->get_size();
   }

   inline tuple_list::iterator erase_from_list(tuple_list *ls,
         tuple_list::iterator& it, mem::node_allocator *alloc,
         const vm::field_type hash_type)
   {
      tuple_list::iterator newit(ls->erase(it));

      if(ls->empty()) {
         tuple_list *prev(ls->prev);
         tuple_list *next(ls->next);
         if(prev)
            prev->next = next;
         else {
            const vm::uint_val id(hash_field(ls->value, hash_type));
            const size_t idx(mod_hash(id));
            table[idx] = next;
         }
         if(next)
            next->prev = prev;
         alloc->deallocate_obj((utils::byte*)ls, sizeof(tuple_list));
         unique_elems--;
      }

      return newit;
   }

   inline tuple_list *lookup_list(const vm::tuple_field field, const vm::field_type hash_type)
   {
      const vm::uint_val id(hash_field(field, hash_type));
      tuple_list *ls(table[mod_hash(id)]);
      while(ls) {
         if(same_field0(field, ls->value, hash_type))
            return ls;
         ls = ls->next;
      }
      return nullptr;
   }

   inline void destroy(mem::node_allocator *alloc)
   {
      if(table)
         alloc->deallocate_obj((utils::byte*)table, sizeof(tuple_list*) * size_table);
      table = nullptr;
   }

   inline void setup(mem::node_allocator *alloc, const size_t default_table_size)
   {
      size_table = default_table_size;
      prime = utils::previous_prime(size_table);
      table = (tuple_list**)alloc->allocate_obj(sizeof(tuple_list*) * size_table);
      memset(table, 0, sizeof(tuple_list*) * size_table);
   }

   inline bool too_sparse() const
   {
      if(flag_expanded) {
         flag_expanded--;
         return false;
      }
      if (unique_elems > (prime*2)/3)
         return false;
      size_t empty_buckets{0};
      for (size_t i(0); i < prime; ++i) {
         tuple_list *ls(table[i]);
         if(!ls || ls->empty())
            empty_buckets++;
         if(empty_buckets >= prime/2)
            return true;
      }
      return false;
   }

   inline void expand(const vm::predicate *pred, mem::node_allocator *alloc, const vm::field_type hash_type) {
      (void)pred;
      const size_t new_size(size_table * 2);
      flag_expanded += 10;
  //   expanded++;
//      std::cout << "expand " << new_size << " " << elems << " " << shrinked << "/" << expanded << "\n";
      change_table(new_size, alloc, hash_type);
   }
   inline void shrink(const vm::predicate *pred, mem::node_allocator *alloc, const vm::field_type hash_type) {
      (void)pred;
      const size_t new_size(size_table / 2);
 //     shrinked++;
//     std::cout << "shrink " << new_size << " " << shrinked << "/" << expanded << "\n";
      change_table(new_size, alloc, hash_type);
   }

   inline void dump(std::ostream& out, const vm::predicate *pred) const
   {
      for (size_t i(0); i < prime; ++i) {
         tuple_list *ls(table[i]);
         out << "Bucket for " << i << ": ";
         while(ls) {
            if (ls->empty())
               out << "empty\n";
            else {
               out << "has " << ls->get_size() << " elements:\n";
               ls->dump(out, pred);
            }
            ls = ls->next;
         }
      }
   }
};

struct hash_table {
   using tuple_list = subhash_table::tuple_list;

   private:

   subhash_table sh;
   size_t elems{0};
   vm::field_type hash_type;

   public:

   inline size_t get_num_buckets(void) const { return sh.size_table; }

   class iterator {
  private:
      tuple_list **bucket;
      tuple_list **finish;
      tuple_list *current;

      inline void find_good_bucket(const bool force_next) {
         if(current) {
            current = current->next;
            if(current)
               return;
            bucket++;
         } else {
            if(force_next)
               bucket++;
         }
         for (; bucket != finish; bucket++) {
            if (*bucket && !(*bucket)->empty()) {
               current = *bucket;
               return;
            }
         }
      }

  public:

      inline bool end(void) const { return bucket == finish && current == nullptr; }

      inline tuple_list *operator*(void) const {
         assert(current);
         return current;
      }

      inline iterator &operator++(void) {
         find_good_bucket(true);
         return *this;
      }

      inline iterator operator++(int) {
         find_good_bucket(true);
         return *this;
      }

      explicit iterator(tuple_list **start_bucket, tuple_list **end_bucket)
          : bucket(start_bucket), finish(end_bucket), current(nullptr) {
         find_good_bucket(false);
      }
   };

   inline iterator begin(void) { return iterator(sh.table, sh.table + sh.prime); }
   inline iterator begin(void) const {
      return iterator(sh.table, sh.table + sh.prime);
   }

   inline size_t get_total_size(void) const { return elems; }

   inline bool empty(void) const { return elems == 0; }

   size_t insert(vm::tuple *item, const vm::predicate *pred, mem::node_allocator *alloc) {
      elems++;
      return sh.insert(item, pred, alloc, hash_type);
   }

   size_t insert_front(vm::tuple *tpl, const vm::predicate* pred) {
      elems++;
      return sh.insert_front(tpl, pred, hash_type);
   }

   inline tuple_list::iterator erase_from_list(tuple_list *ls,
         tuple_list::iterator& it, mem::node_allocator *alloc)
   {
      elems--;
      return sh.erase_from_list(ls, it, alloc, hash_type);
   }

   inline tuple_list *lookup_list(
       const vm::tuple_field field)
   {
      return sh.lookup_list(field, hash_type);
   }

   inline void dump(std::ostream &out, const vm::predicate *pred) const {
      sh.dump(out, pred);
   }

   inline void expand(const vm::predicate *pred, mem::node_allocator *alloc) {
      return sh.expand(pred, alloc, hash_type);
   }

   inline void shrink(const vm::predicate *pred, mem::node_allocator *alloc) {
      return sh.shrink(pred, alloc, hash_type);
   }

   inline bool smallest_possible(void) const {
      return sh.size_table == HASH_TABLE_INITIAL_TABLE_SIZE;
   }

   inline bool too_sparse(void) const {
      return sh.too_sparse();
   }

   static inline vm::tuple_list *underlying_list(tuple_list *ls)
   {
      if(!ls)
         return nullptr;
      return &(ls->list);
   }

   static inline tuple_list *cast_list(vm::tuple_list *ls)
   {
      return (tuple_list*)ls;
   }

   inline void setup(const vm::field_type type, mem::node_allocator *alloc,
       const size_t default_table_size = HASH_TABLE_INITIAL_TABLE_SIZE) {
      hash_type = type;
      sh.setup(alloc, default_table_size);
   }

   inline void destroy(mem::node_allocator *alloc) {
      sh.destroy(alloc);
   }
};
}

#endif
