
#ifndef MEM_POOL_HPP
#define MEM_POOL_HPP

#include <atomic>
#include <vector>
#include <iostream>
#include <assert.h>
#include <cstdio>
#include <cstring>

#include "mem/chunkgroup.hpp"
#include "mem/mem_node.hpp"

namespace mem {

struct pool {
   private:
   chunkgroup chunk_table[31];
   size_t size_table;

   // available page for chunkgroups.
   chunkgroup *next_group;
   chunkgroup *end_groups;
   chunkgroup *free_groups;

   // number of chunkgroups to allocate per page.
   static const size_t NUM_CHUNK_PAGES = 64;

   chunkgroup available_groups[NUM_CHUNK_PAGES];
   chunkgroup __pad;

   inline void create_new_chunkgroup_page(void) {
      const size_t size_groups(sizeof(chunkgroup) * NUM_CHUNK_PAGES);
      next_group = (chunkgroup *)new unsigned char[size_groups];
      end_groups = next_group + NUM_CHUNK_PAGES;
   }

   inline chunkgroup *new_chunkgroup(const size_t size) {
      if (free_groups) {
         chunkgroup *ret(free_groups);
         free_groups = free_groups->next;
         ret->init(size);
         return ret;
      }
      if (next_group == end_groups) create_new_chunkgroup_page();
      chunkgroup *next(next_group);
      next_group++;
      ::new ((void *)next) chunkgroup(size);
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
      ::new ((void *)next) chunkgroup();
      return next;
   }

#if 0
   inline void expand_chunk_table(void) {
      chunkgroup *old_table(chunk_table);
      const size_t old_size(size_table);

      size_table *= 2;
      chunk_table = new chunkgroup[size_table + 8];
      memset(chunk_table, 0, sizeof(chunkgroup) * size_table);

      for (size_t i(0); i < old_size; ++i) {
         chunkgroup *bucket(old_table + i);
         bool first{true};
         while (bucket) {
            chunkgroup *next(bucket->next);
            const size_t new_index(hash_size(bucket->size) % size_table);
            chunkgroup *new_bucket(chunk_table + new_index);
            if (new_bucket->size == 0) {
               memcpy(new_bucket, bucket, sizeof(chunkgroup));
               new_bucket->next = NULL;
               if (!first) {
                  bucket->next = free_groups;
                  free_groups = bucket;
               }
            } else {
               if (first) {
                  chunkgroup *copy(new_chunkgroup());
                  memcpy(copy, bucket, sizeof(chunkgroup));
                  copy->next = new_bucket->next;
                  new_bucket->next = copy;
               } else {
                  bucket->next = new_bucket->next;
                  new_bucket->next = bucket;
               }
            }
            first = false;

            bucket = next;
         }
      }

      delete[] old_table;
   }
#endif

   inline chunkgroup *find_insert_chunkgroup(const size_t size) {
      // tries to find the chunkgroup, if not add it.
      const size_t index(hash_size(size) % size_table);
      chunkgroup *place(chunk_table + index);

      if (place->size == 0) {
         place->init(size);
         return place;
      } else {
         size_t count(0);

         while (place) {
            if (place->size == size) return place;
            count++;
            place = place->next;
         }

#if 0
         if(count > 8) // time to expand table
         {
            expand_chunk_table();
         }
         XXX
#endif

         chunkgroup *ncg(new_chunkgroup(size));
         ncg->next = chunk_table[index].next;
         chunk_table[index].next = ncg;

         return ncg;
      }
   }

   inline chunkgroup *get_group(const size_t size) {
      assert(size > 0);
      chunkgroup *cg(find_insert_chunkgroup(size));
      assert(cg != nullptr);
      return cg;
   }

   inline size_t hash_size(const size_t size) { return (size >> 2) - 1; }

   inline size_t make_size(const size_t size) {
      return std::max(MEM_MIN_OBJS, size);
   }

   public:
#ifdef INSTRUMENTATION
   std::atomic<int64_t> bytes_in_use{0};
#endif

   inline void *allocate(const size_t size) {
      const size_t new_size(make_size(size));
      chunkgroup *grp(get_group(new_size));
#ifdef INSTRUMENTATION
      bytes_in_use += new_size;
#endif
      return grp->allocate();
   }

   inline void deallocate(void *ptr, const size_t size) {
      const size_t new_size(make_size(size));
      chunkgroup *grp(get_group(new_size));
#ifdef INSTRUMENTATION
      bytes_in_use -= new_size;
#endif
      return grp->deallocate(ptr);
   }

   inline void create() {
      size_table = 31;
      next_group = available_groups;
      free_groups = nullptr;
      end_groups = next_group + NUM_CHUNK_PAGES;
      memset(chunk_table, 0, sizeof(chunkgroup) * size_table);
   }

   void dump() {
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
   }
};

}

#endif
