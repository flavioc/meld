
#ifndef MEM_CHUNKGROUP_HPP
#define MEM_CHUNKGROUP_HPP

#include <limits>

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
   mem_node *free_objs; // list of freed objects
   
   static const size_t INITIAL_NUM_ELEMS = 64;
   size_t num_elems_per_chunk;

public:
   
   inline void* allocate(void)
   {
      void *ret;
      
      if(free_objs != NULL) {
         // use a free chunk node
         ret = free_objs;
         free_objs = free_objs->next;
         return ret;
      }
      
      if(new_chunk == NULL) {
         // this is the first chunk
         new_chunk = first_chunk = new chunk(size, num_elems_per_chunk);
         return new_chunk->allocate(size);
      }

      ret = new_chunk->allocate(size);

      if(ret == NULL) { // chunk full!
         chunk *old_chunk(new_chunk);
         if(num_elems_per_chunk < std::numeric_limits<std::size_t>::max()/2)
            num_elems_per_chunk *= 2; // increase number of elements
         new_chunk = new chunk(size, num_elems_per_chunk);
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
      
      new_node->next = free_objs;
      free_objs = new_node;
   }
   
   explicit chunkgroup(const size_t _size):
      size(_size), first_chunk(NULL),
      new_chunk(NULL), free_objs(NULL),
      num_elems_per_chunk(INITIAL_NUM_ELEMS)
   {
   }
   
   ~chunkgroup(void)
   {
      chunk *cur(first_chunk);
      
      while (cur) {
         chunk *next(cur->next_chunk);
         delete cur;
         cur = next;
      }
   }
};

}

#endif

