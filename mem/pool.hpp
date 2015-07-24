
#ifndef MEM_POOL_HPP
#define MEM_POOL_HPP

#include <atomic>
#include <vector>
#include <iostream>
#include <assert.h>
#include <cstdio>
#include <cstring>

#include "mem/chunkgroup.hpp"
#include "mem/mixedgroup.hpp"
#include "mem/mem_node.hpp"
#include "mem/bigchunk.hpp"

#define MIXED_MEM

#define ALLOCATOR_MAX_SIZE (std::numeric_limits<uint16_t>::max())

namespace mem {

struct pool {
   private:

   mixedgroup conses;
#ifdef MIXED_MEM
   mixedgroup small;
   mixedgroup medium;
   mixedgroup large;
#else
   bigchunk bc;
   chunkgroup chunk_table[31];
   size_t size_table;

   // available page for chunkgroups.
   chunkgroup *next_group;
   chunkgroup *end_groups;
   chunkgroup *free_groups;

   // number of chunkgroups to allocate per page.
   static const size_t NUM_CHUNK_PAGES = 32;

   chunkgroup available_groups[NUM_CHUNK_PAGES];
#endif
   chunkgroup __pad;

#ifndef MIXED_MEM
   inline void create_new_chunkgroup_page(void) {
      const size_t size_groups(sizeof(chunkgroup) * NUM_CHUNK_PAGES);
      next_group = (chunkgroup *)new unsigned char[size_groups];
      register_malloc(size_groups);
      end_groups = next_group + NUM_CHUNK_PAGES;
   }

   inline chunkgroup *new_chunkgroup(const size_t size) {
      if (free_groups) {
         chunkgroup *ret(free_groups);
         free_groups = free_groups->next;
         init_chunkgroup(ret, size);
         return ret;
      }
      if (next_group == end_groups) create_new_chunkgroup_page();
      chunkgroup *next(next_group);
      init_chunkgroup(next, size);
      next_group++;
      return next;
   }

   inline chunkgroup *new_chunkgroup() {
      if (free_groups) {
         chunkgroup *ret(free_groups);
         free_groups = free_groups->next;
         return ret;
      }
      if (next_group == end_groups) create_new_chunkgroup_page();
      chunkgroup *next(next_group);
      next_group++;
      return next;
   }
#endif

#ifndef MIXED_MEM
   inline void init_chunkgroup(chunkgroup *cg, const size_t size)
   {
      cg->init(size, &bc);
      chunkgroup *twice(find_chunkgroup(size * 2));
      chunkgroup *half(find_chunkgroup(size / 2));
      cg->set_twice(twice);
      if(half)
         half->set_twice(cg);
   }

   inline chunkgroup *find_chunkgroup(const size_t size) {
      // tries to find the chunkgroup
      const size_t index(hash_size(size) % size_table);
      chunkgroup *place(chunk_table + index);

      if (place->size == 0)
         return nullptr;
      else {
         size_t count(0);

         while (place) {
            if (place->size == size) return place;
            count++;
            place = place->next;
         }

         return nullptr;
      }
   }
   inline chunkgroup *find_insert_chunkgroup(const size_t size) {
      // tries to find the chunkgroup, if not add it.
      const size_t index(hash_size(size) % size_table);
      chunkgroup *place(chunk_table + index);

      if (place->size == 0) {
         init_chunkgroup(place, size);
         return place;
      } else {
         size_t count(0);

         while (place) {
            if (place->size == size) return place;
            count++;
            place = place->next;
         }

         chunkgroup *ncg(new_chunkgroup(size));
         ncg->next = chunk_table[index].next;
         chunk_table[index].next = ncg;

         return ncg;
      }
   }

   inline chunkgroup *get_group(const size_t size) {
      assert(size > 0);
      chunkgroup *cg(find_insert_chunkgroup(size));
      assert(cg);
      return cg;
   }

   inline size_t hash_size(const size_t size) { return (size >> 2) - 1; }
#endif
   inline size_t make_size(size_t size) {
      size = std::max(MEM_MIN_OBJS, size);
      while(size % 4 != 0)
         size++;
      return size;
   }

   public:

#ifdef INSTRUMENTATION
   std::atomic<int64_t> bytes_in_use{0};
#endif

   inline void *allocate_cons(const size_t size) {
      return conses.allocate(size);
   }

   inline void *allocate(const size_t size) {
      const size_t new_size(make_size(size));
#if 0
      if(new_size >= ALLOCATOR_MAX_SIZE) {
         register_malloc(new_size * sizeof(utils::byte));
         return malloc(new_size * sizeof(utils::byte));
      }
#endif

#ifdef MIXED_MEM
      if(new_size <= 128)
         return small.allocate(new_size);
      else if(new_size <= 1024)
         return medium.allocate(new_size);
      else
         return large.allocate(new_size);
#else
      chunkgroup *grp(get_group(new_size));
#ifdef INSTRUMENTATION
      bytes_in_use += new_size;
#endif
      if(grp->has_free())
         return grp->allocate_free();
#if 0
      chunkgroup *twice(grp->get_twice());
      if(twice && twice->has_free()) {
         utils::byte *ptr((utils::byte*)twice->allocate_free());
         utils::byte *other_half(ptr + new_size);
         grp->deallocate(other_half);
         return ptr;
      }
#endif
      return grp->allocate(&bc);
#endif
   }

   inline void deallocate_cons(void *ptr, const size_t size) {
      conses.deallocate(ptr, size);
   }

   inline void deallocate(void *ptr, const size_t size) {
      const size_t new_size(make_size(size));
#if 0
      if(new_size >= ALLOCATOR_MAX_SIZE) {
         free(ptr);
         return;
      }
#endif
#ifdef MIXED_MEM
      if(new_size <= 128)
         small.deallocate(ptr, new_size);
      else if(new_size <= 1024)
         medium.deallocate(ptr, new_size);
      else
         large.deallocate(ptr, new_size);
#else
      chunkgroup *grp(get_group(new_size));
#ifdef INSTRUMENTATION
      bytes_in_use -= new_size;
#endif
      return grp->deallocate(ptr);
#endif
   }

   inline void create() {
      conses.create(8);
#ifdef MIXED_MEM
      small.create(4);
      medium.create(8);
      large.create(32);
#else
      size_table = 31;
      next_group = available_groups;
      free_groups = nullptr;
      end_groups = next_group + NUM_CHUNK_PAGES;
      memset(chunk_table, 0, sizeof(chunkgroup) * size_table);
      bc.create();
#endif
   }

   void dump() {
#ifndef MIXED_MEM
      std::cout << "--> SIZE TABLE " << size_table << " <--\n";
      for (size_t i(0); i < size_table; ++i) {
         chunkgroup *bucket = chunk_table + i;
         if (bucket->size == 0) continue;
         std::cout << "bucket " << i << std::endl;
         std::cout << "\t";
         while (bucket) {
            std::cout << bucket->size << " ";
            bucket = bucket->next;
         }
         std::cout << "\n";
      }
#endif
   }
};

}

#endif
