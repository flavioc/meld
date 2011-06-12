
#ifndef MEM_POOL_HPP
#define MEM_POOL_HPP

#include <vector>
#include <iostream>
#include <assert.h>
#include <cstdio>
#include <tr1/unordered_map>

#include "mem/chunkgroup.hpp"

namespace mem
{

class pool
{
private:
   
   static const size_t ATOM_SIZE = 4;
   
   typedef std::tr1::unordered_map<size_t, chunkgroup*> chunk_map;

   chunk_map chunks;
   
   chunkgroup *get_group(const size_t size)
   {
      assert(size > 0);
        
      chunk_map::iterator it(chunks.find(size));
      chunkgroup *grp;
      
      if(it == chunks.end()) {
         grp = new chunkgroup(size);
         chunks[size] = grp;
      } else
         grp = it->second;
         
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
   }
   
   ~pool(void)
   {
      for(chunk_map::iterator it(chunks.begin());
         it != chunks.end();
         ++it)
      {
         delete it->second;
      }
   }
};

}

#endif
