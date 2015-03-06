
#ifndef MEM_POOL_HPP
#define MEM_POOL_HPP

#include <atomic>
#include <vector>
#include <iostream>
#include <assert.h>
#include <cstdio>
#include <cstring>

#include "mem/chunkgroup.hpp"

namespace mem
{

class pool
{
private:
   
   chunkgroup **chunk_table;
   size_t size_table = 127; // default table size

   // number of chunkgroups to allocate per page.
   static const size_t NUM_CHUNK_PAGES = 64;

   // available page for chunkgroups.
   chunkgroup *available_groups;
   chunkgroup *end_groups;

   inline void create_new_chunkgroup_page(void)
   {
      const size_t size_groups(sizeof(chunkgroup)*NUM_CHUNK_PAGES);
      available_groups = (chunkgroup*) new unsigned char[size_groups];
      end_groups = available_groups + NUM_CHUNK_PAGES;
   }

   inline chunkgroup *new_chunkgroup(const size_t size)
   {
      if(available_groups == end_groups)
         create_new_chunkgroup_page();
      chunkgroup *next(available_groups);
      available_groups++;
      ::new((void *)next) chunkgroup(size);
      return next;
   }

   inline void expand_chunk_table(void)
   {
      chunkgroup **old_table(chunk_table);
      const size_t old_size(size_table);

      size_table *= 2;
      chunk_table = new chunkgroup*[size_table];
      memset(chunk_table, 0, sizeof(chunkgroup*)*size_table);

      for(size_t i(0); i < old_size; ++i) {
         chunkgroup *bucket(old_table[i]);
         while(bucket) {
            chunkgroup *next(bucket->next);
            const size_t new_index(bucket->size % size_table);
            bucket->next = chunk_table[new_index];
            chunk_table[new_index] = bucket;

            bucket = next;
         }
      }

      delete[] old_table;
   }

   inline chunkgroup *find_insert_chunkgroup(const size_t size)
   {
      // tries to find the chunkgroup, if not add it.
      const size_t index(size % size_table);
      
      if(chunk_table[index] == nullptr) {
         chunkgroup *cg(new_chunkgroup(size));
         chunk_table[index] = cg;
         cg->next = nullptr;
         return cg;
      } else {
         chunkgroup *cg(chunk_table[index]);
         size_t count(0);

         while(cg) {
            if(cg->size == size)
               return cg;
            count++;
            cg = cg->next;
         }

         chunkgroup *ncg(new_chunkgroup(size));
         ncg->next = chunk_table[index];
         chunk_table[index] = ncg;

         if(count > 8) // time to expand table
         {
            expand_chunk_table();
         }
         return ncg;
      }
   }

   inline chunkgroup *get_group(const size_t size)
   {
      assert(size > 0);
      chunkgroup *cg(find_insert_chunkgroup(size));
      assert(cg != nullptr);
      return cg;
   }

   inline size_t make_size(const size_t size)
   {
      return std::max(sizeof(intptr_t), size);
   }

public:

#ifdef INSTRUMENTATION
   std::atomic<int64_t> bytes_in_use{0};
#endif
   
   inline void* allocate(const size_t size)
   {
      const size_t new_size(make_size(size));
      chunkgroup *grp(get_group(new_size));
#ifdef INSTRUMENTATION
      bytes_in_use += new_size;
#endif
      return grp->allocate();
   }
   
   inline void deallocate(void *ptr, const size_t size)
   {
      const size_t new_size(make_size(size));
      chunkgroup *grp(get_group(new_size));
#ifdef INSTRUMENTATION
      bytes_in_use -= new_size;
#endif
      return grp->deallocate(ptr);
   }
   
   explicit pool(void)
   {
      create_new_chunkgroup_page();
      chunk_table = new chunkgroup*[size_table];
      memset(chunk_table, 0, sizeof(chunkgroup*)*size_table);
   }
};

}

#endif
