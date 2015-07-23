
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
#include "vm/bitmap_static.hpp"

namespace db {

#define HASH_TABLE_SUBHASH_SHIFT 5
#define HASH_TABLE_PRIME 29
#define HASH_TABLE_INITIAL_TABLE_SIZE (1 << HASH_TABLE_SUBHASH_SHIFT)

static_assert(HASH_TABLE_PRIME < HASH_TABLE_INITIAL_TABLE_SIZE, "prime < size");
static_assert(sizeof(BITMAP_TYPE) * 8 >= HASH_TABLE_INITIAL_TABLE_SIZE, "wrong table size");

#define CREATE_HASHTABLE_THREADSHOLD 8
#define HASH_TABLE_MAX_LEVELS 4
#define HASH_TABLE_SUBHASH_MASK ((1 << HASH_TABLE_SUBHASH_SHIFT) - 1)

struct subhash_table;

struct hash_table_list {
   vm::tuple_list list;
   vm::tuple_field value;
   subhash_table *parent;
   std::uint16_t idx;

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

static inline vm::uint_val hash_field(const vm::tuple_field field, const vm::field_type hash_type) {
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
 //         return (vm::uint_val)*(vm::node_val*)data;
            return utils::fnv1_hash((utils::byte*)&data, sizeof(vm::uint_val));
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

static inline vm::uint_val hash_tuple(vm::tuple *tpl, const vm::predicate* pred, const vm::field_type hash_type) {
   return hash_field(tpl->get_field(pred->get_hashed_field()), hash_type);
}

struct subhash_table {
   using tuple_list = hash_table_list;
   tuple_list *table[HASH_TABLE_INITIAL_TABLE_SIZE];
   vm::bitmap_static<1> bitmap;
   std::uint16_t unique_lists{0};
   std::uint16_t unique_subs{0};
   subhash_table *parent;

   inline uint64_t mod_hash(const uint64_t hsh) {
      return hsh % HASH_TABLE_PRIME;
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

   inline tuple_list *get_bucket(const size_t idx) const
   {
      return table[idx];
   }

   inline subhash_table *get_subhash(const size_t idx)
   {
      return (subhash_table*)table[idx];
   }

   inline bool is_subhash(const size_t idx)
   {
      return bitmap.get_bit(idx);
   }

   inline void set_subhash(const size_t idx, subhash_table *p)
   {
      bitmap.set_bit(idx);
      p->table[HASH_TABLE_INITIAL_TABLE_SIZE - 1] = (tuple_list*)(std::uintptr_t)idx;
      table[idx] = (tuple_list*)p;
      assert(is_subhash(idx));
   }

public:

   inline size_t get_table_size() const { return HASH_TABLE_INITIAL_TABLE_SIZE; }

   size_t insert(const vm::uint_val id, vm::tuple *item, const vm::predicate *pred,
         mem::node_allocator *alloc, const vm::field_type hash_type, const size_t level)
   {
      const vm::uint_val subid(id & HASH_TABLE_SUBHASH_MASK);
      const size_t idx(mod_hash(subid));
      if(is_subhash(idx)) {
         assert(unique_subs > 0);
         return get_subhash(idx)->insert(id >> HASH_TABLE_SUBHASH_SHIFT, item, pred, alloc, hash_type, level + 1);
      }

      tuple_list *bucket(get_bucket(idx));
      assert(!is_subhash(idx));
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
      newls->prev = last;
      if(last)
         last->next = newls;
      else
         table[idx] = newls;
      newls->idx = idx;
      newls->value = item->get_field(pred->get_hashed_field());
      newls->next = nullptr;
      newls->push_back(item);
      newls->parent = this;
      unique_lists++;
      if(count + 1 >= CREATE_HASHTABLE_THREADSHOLD && level + 1 < HASH_TABLE_MAX_LEVELS) {
         // create sub hash table
         subhash_table *sub((subhash_table*)alloc->allocate_obj(sizeof(subhash_table)));
         sub->setup(this);
         tuple_list *bucket(get_bucket(idx));
         size_t total{0};

         while(bucket) {
            tuple_list *next(bucket->next);
            vm::uint_val hsh(hash_field(bucket->value, hash_type));
            hsh >>= (level + 1) * HASH_TABLE_SUBHASH_SHIFT;
            hsh &= HASH_TABLE_SUBHASH_MASK;
            const vm::uint_val index(sub->mod_hash(hsh));
            bucket->parent = sub;
            if(sub->table[index])
               sub->table[index]->prev = bucket;
            bucket->next = sub->table[index];
            sub->table[index] = bucket;
            bucket->prev = nullptr;
            bucket->idx = (std::uint16_t)index;

            bucket = next;
            total++;
         }
         sub->unique_lists = total;
         assert(unique_lists >= total);
         unique_lists -= total;
         unique_subs++;
         set_subhash(idx, sub);
         assert(is_subhash(idx));
      }
      return newls->get_size();
   }

   inline size_t find_level() const
   {
      if(parent == nullptr)
         return 0;
      else
         return 1 + parent->find_level();
   }

   inline tuple_list::iterator erase_from_list(tuple_list *ls,
         tuple_list::iterator& it, mem::node_allocator *alloc,
         const vm::field_type hash_type)
   {
      (void)hash_type;
      (void)alloc;
      tuple_list::iterator newit(ls->erase(it));

      if(ls->empty()) {
         tuple_list *prev(ls->prev);
         tuple_list *next(ls->next);
         if(prev)
            prev->next = next;
         else
            table[ls->idx] = next;
         if(next)
            next->prev = prev;
         alloc->deallocate_obj((utils::byte*)ls, sizeof(tuple_list));
         assert(unique_lists >= 1);
         unique_lists--;
         check_empty_table(alloc);
      }

      return newit;
   }

   inline void check_empty_table(mem::node_allocator *alloc) {
      if(unique_lists == 0 && unique_subs == 0 && parent != nullptr) {
         subhash_table *p(parent);
         // sub hash table is empty, delete it!
         std::uint16_t idx_parent((std::uintptr_t)table[HASH_TABLE_INITIAL_TABLE_SIZE-1]);
         p->table[idx_parent] = nullptr;
         p->bitmap.unset_bit(idx_parent);
         p->unique_subs--;
         alloc->deallocate_obj((utils::byte*)this, sizeof(subhash_table));

         p->check_empty_table(alloc);
      }
   }

   inline tuple_list *lookup_list(const vm::uint_val id, const vm::tuple_field field, const vm::field_type hash_type)
   {
      const size_t idx(mod_hash(id & HASH_TABLE_SUBHASH_MASK));
      if(is_subhash(idx)) {
         assert(unique_subs > 0);
         return get_subhash(idx)->lookup_list(id >> HASH_TABLE_SUBHASH_SHIFT, field, hash_type);
      }

      tuple_list *ls(get_bucket(idx));
      while(ls) {
         if(same_field0(field, ls->value, hash_type))
            return ls;
         ls = ls->next;
      }
      return nullptr;
   }

   inline void destroy(mem::node_allocator *alloc)
   {
      for(size_t i(0); i < HASH_TABLE_PRIME; ++i) {
         if(is_subhash(i)) {
            assert(unique_subs > 0);
            get_subhash(i)->destroy(alloc);
            alloc->deallocate_obj((utils::byte*)get_subhash(i), sizeof(subhash_table));
         } else {
            tuple_list *ls(table[i]);
            while(ls) {
               tuple_list *next(ls->next);
               alloc->deallocate_obj((utils::byte*)ls, sizeof(tuple_list));
               ls = next;
            }
         }
      }
   }

   inline void setup(subhash_table *par)
   {
      memset(table, 0, sizeof(tuple_list*) * HASH_TABLE_INITIAL_TABLE_SIZE);
      parent = par;
      unique_lists = 0;
      unique_subs = 0;
   }

   inline void dump(std::ostream& out, const vm::predicate *pred) const
   {
      for (size_t i(0); i < HASH_TABLE_PRIME; ++i) {
         tuple_list *ls(get_bucket(i));
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

   subhash_table *sh;
   size_t elems{0};
   vm::field_type hash_type;

   public:

   class iterator {
  private:
     struct frame {
        subhash_table *sub;
        size_t pos;
     };
     tuple_list *current;

     frame frames[HASH_TABLE_MAX_LEVELS];
     int cur_frame;

      inline void find_good_bucket(const bool force_next) {
         if(current) {
            current = current->next;
            if(current)
               return;
            frame& f(frames[cur_frame]);
            f.pos++;
         } else {
            if(force_next) {
               frame& f(frames[cur_frame]);
               f.pos++;
            }
         }
         for(; cur_frame >= 0; cur_frame--) {
            frame *f(&(frames[cur_frame]));
            subhash_table *s(f->sub);
            for (; f->pos < HASH_TABLE_PRIME; (f->pos)++) {
reset:
               size_t pos(f->pos);
               if(s->is_subhash(pos)) {
//                  std::cout << "Go to frame " << cur_frame + 1 << "\n";
                  f->pos++;
                  cur_frame++;
                  assert(cur_frame >= 0 && cur_frame <= HASH_TABLE_MAX_LEVELS);
                  f++;
                  f->sub = s->get_subhash(pos);
                  f->pos = 0;
                  s = f->sub;
                  goto reset;
               }

               current = s->get_bucket(pos);
               if(current)
                  return;
            }
         }
      }

  public:

      inline bool end(void) const { return cur_frame == -1; }

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

      explicit iterator(subhash_table *sub): current(nullptr) {
         cur_frame = 0;
         frames[0].sub = sub;
         frames[0].pos = 0;
         find_good_bucket(false);
      }
   };

   inline iterator begin(void) { return iterator((subhash_table*)sh); }
   inline iterator begin(void) const { return iterator((subhash_table*)sh); }

   inline size_t get_total_size(void) const { return elems; }

   inline bool empty(void) const { return elems == 0; }

   size_t insert(vm::tuple *item, const vm::predicate *pred, mem::node_allocator *alloc) {
      const vm::uint_val id(hash_tuple(item, pred, hash_type));
      elems++;
//    std::cout << "Insert ==> " << id << "\n";
      const size_t ls(sh->insert(id, item, pred, alloc, hash_type, 0) == 1);
      return ls;
   }

   inline tuple_list::iterator erase_from_list(tuple_list *ls,
         tuple_list::iterator& it, mem::node_allocator *alloc)
   {
      elems--;
      subhash_table *sub(ls->parent);
      assert(sub);
      return sub->erase_from_list(ls, it, alloc, hash_type);
   }

   inline tuple_list *lookup_list(
       const vm::tuple_field field)
   {
      const vm::uint_val id(hash_field(field, hash_type));
      //std::cout << "Found ==> " << id << "\n";
      return sh->lookup_list(id, field, hash_type);
   }

   inline void dump(std::ostream &out, const vm::predicate *pred) const {
      sh->dump(out, pred);
   }

   inline bool too_sparse() const
   {
      return sh->unique_lists < CREATE_HASHTABLE_THREADSHOLD/2 && sh->unique_subs == 0;
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

   static inline tuple_list *cast_list(tuple_list *ls)
   {
      return ls;
   }

   inline void setup(const vm::field_type type, mem::node_allocator *alloc)
   {
      hash_type = type;
      elems = 0;
      sh = (subhash_table*)alloc->allocate_obj(sizeof(subhash_table));
      sh->setup(nullptr);
   }

   inline void destroy(mem::node_allocator *alloc) {
      assert(sh);
      sh->destroy(alloc);
      alloc->deallocate_obj((utils::byte*)sh, sizeof(subhash_table));
   }
};
}

#endif
