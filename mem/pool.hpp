
#ifndef MEM_POOL_HPP
#define MEM_POOL_HPP

#include <vector>
#include <iostream>
#include <assert.h>
#include <cstdio>
#include <unordered_map>

#include "mem/chunkgroup.hpp"
#include "conf.hpp"

namespace mem
{

#define MAX_CHUNK_SIZE 2048

class pool
{
private:
   
   static const size_t ATOM_SIZE = 4;
   
   chunkgroup *chunks[MAX_CHUNK_SIZE];

   inline chunkgroup *get_group(const size_t size)
   {
      assert(size > 0);
      assert(size < MAX_CHUNK_SIZE);
        
      chunkgroup *grp = chunks[size];
      
      if(grp == NULL) {
         grp = new chunkgroup(size);
         chunks[size] = grp;
      }
         
      assert(grp != NULL);
      
      return grp;
   }

public:
   
   inline void* allocate(const size_t size)
   {
      return get_group(size)->allocate();
   }
   
   inline void deallocate(void *ptr, const size_t size)
   {
      return get_group(size)->deallocate(ptr);
   }
   
   explicit pool(void)
   {
      memset(chunks, 0, sizeof(chunks));
   }
   
   ~pool(void)
   {
      for(size_t i(1); i < MAX_CHUNK_SIZE; ++i) {
         if(chunks[i] != NULL)
            delete chunks[i];
      }
   }
};

}

#endif
