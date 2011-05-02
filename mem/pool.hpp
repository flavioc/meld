
#ifndef MEM_POOL_HPP
#define MEM_POOL_HPP

#include <vector>
#include <assert.h>
#include <cstdio>

#include "mem/chunkgroup.hpp"

namespace mem
{

class pool
{
private:
   
   static const size_t ATOM_SIZE = 4;
   static const size_t MAX_OBJECT = 512;
   static const size_t MAX_OBJECT_SIZE = MAX_OBJECT * ATOM_SIZE;
   
   typedef std::vector<chunkgroup*> chunk_vector;
   
   chunk_vector chunks;
   
   size_t get_group(const size_t size)
   {
      assert(size != 0);
      if(size > MAX_OBJECT_SIZE) {
         printf("%ld\n", size);
         exit(1);
      }
      assert(size <= MAX_OBJECT_SIZE);
      
      assert(size % 4 == 0);
      
      const size_t place((size / ATOM_SIZE) - 1);
      
      assert(place < chunks.size());
      
      return place;
   }

public:
   
   inline void* allocate(const size_t size)
   {
      //printf("ALLOCATE SIZE: %ld\n", size);
      const size_t grp(get_group(size));
      //printf("Group %d\n", size / ATOM_SIZE - 1);
      
      return chunks[grp]->allocate();
   }
   
   inline void deallocate(void *ptr, const size_t size)
   {
      const size_t grp(get_group(size));
      
      //printf("DEALLOCATE SIZE %ld (%p)\n", size, ptr);
      
      chunks[grp]->deallocate(ptr);
   }
   
   explicit pool(void)
   {
      chunks.reserve(MAX_OBJECT);
      
      for(size_t i(0); i < MAX_OBJECT; ++i)
         chunks.push_back(new chunkgroup((i+1)*ATOM_SIZE));
   }
   
   ~pool(void)
   {
      for(chunk_vector::iterator it(chunks.begin());
         it != chunks.end();
         ++it)
      {
         chunkgroup *grp(*it);
         delete grp;
      }
   }
};

}

#endif
