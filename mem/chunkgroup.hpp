
#ifndef MEM_CHUNKGROUP_HPP
#define MEM_CHUNKGROUP_HPP

#include "mem/chunk.hpp"

namespace mem
{

class chunkgroup
{
private:

   struct mem_node {
      struct mem_node *next;
   };

   const size_t size;
   chunk *first_chunk;
   chunk *new_chunk; // chunk with readily usable objects
   mem_node *free_chunk; // list of freed objects

public:
   
   inline void* allocate(void)
   {
      void *ret;
      
      if(free_chunk != NULL) {
         // use a free chunk node
         ret = free_chunk;
         free_chunk = free_chunk->next;
         return ret;
      }
      
      if(new_chunk == NULL) {
         // this is the first chunk
         new_chunk = first_chunk = new chunk(size);
         return new_chunk->allocate(size);
      }

      ret = new_chunk->allocate(size);

      if(ret == NULL) { // chunk full!
         chunk *old_chunk(new_chunk);
         new_chunk = new chunk(size);
         old_chunk->set_next(new_chunk);
         return new_chunk->allocate(size);
      } else {
         // use returned chunk
         return ret;
      }
   }
   
   inline void deallocate(void* ptr)
   {
      mem_node *new_node((mem_node*)ptr);
      
      //printf("Adding new node (deallocate)\n");
      new_node->next = free_chunk;
      free_chunk = new_node;
   }
   
   explicit chunkgroup(const size_t _size):
      size(_size), first_chunk(NULL),
      new_chunk(NULL), free_chunk(NULL)
   {
   }
   
   ~chunkgroup(void)
   {
      // this will delete everything else
      delete first_chunk;
   }
};

}

#endif

