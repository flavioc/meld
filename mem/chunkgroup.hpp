
#ifndef MEM_CHUNKGROUP_HPP
#define MEM_CHUNKGROUP_HPP

#include <list>

#include "mem/chunk.hpp"

namespace mem
{

class chunkgroup
{
private:

   struct mem_node {
      struct mem_node *next;
   };
   
   typedef std::list<chunk*> chunk_list;

   const size_t size;
   chunk_list used_chunks; // chunks already processed
   chunk *new_chunk; // chunk with readily usable objects
   mem_node *free_chunk; // list of freed objects

public:
   
   inline void* allocate(void)
   {
      void *ret;
      //printf(" -> size: %d\n", size);
      
      if(free_chunk != NULL) {
         //printf("Using a free chunk node\n");
         ret = free_chunk;
         free_chunk = free_chunk->next;
         return ret;
      }
      
      if(new_chunk == NULL) {
         //printf("Creating first new chunk\n");
         new_chunk = new chunk(size);
         return new_chunk->allocate(size);
      }

      ret = new_chunk->allocate(size);

      if(ret == NULL) { // chunk full!
         //printf("Chunk full, now %d\n", used_chunks.size()+1);
         used_chunks.push_back(new_chunk);
         new_chunk = new chunk(size);
         return new_chunk->allocate(size);
      } else {
         //printf("Using from new chunk\n");
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
      size(_size), new_chunk(NULL), free_chunk(NULL)
   {
   }
   
   ~chunkgroup(void)
   {
      for(chunk_list::iterator it(used_chunks.begin());
         it != used_chunks.end();
         ++it)
      {
         //printf("Deleted here\n");
         delete *it;
      }
      
      if(new_chunk != NULL)
         delete new_chunk;
   }
};

}

#endif

