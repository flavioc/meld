
#ifndef MEM_CHUNKGROUP_HPP
#define MEM_CHUNKGROUP_HPP

#include <limits>
#include <assert.h>

#include "mem/mem_node.hpp"
#include "mem/chunk.hpp"

namespace mem
{

struct chunkgroup
{
public:
   // next pointer used by mem/pool.
   chunkgroup *next;
   size_t size;
   
private:

   chunk *first_chunk;
   chunk *new_chunk; // chunk with readily usable objects
   
   size_t num_elems_per_chunk;

   free_queue free;

public:

   inline void* allocate()
   {
      void *ret;

#ifndef USE_REFCOUNT
      if(!free_queue_empty(free))
         return free_queue_pop(free);
#else
      chunk *c(first_chunk);
      while(c) {
         if(c->has_free())
            return c->allocate_free();
         c = c->next_chunk;
      }
#endif
      
      if(new_chunk == nullptr) {
         // this is the first chunk
         new_chunk = first_chunk = chunk::create(size, num_elems_per_chunk, nullptr);
         return new_chunk->allocate_new(size);
      }

      ret = new_chunk->allocate_new(size);

      if(ret)
         return ret;

      chunk *old_chunk(new_chunk);
      if(num_elems_per_chunk < std::numeric_limits<std::size_t>::max()/2)
         num_elems_per_chunk *= 2; // increase number of elements
      new_chunk = chunk::create(size, num_elems_per_chunk, old_chunk);
      old_chunk->set_prev(new_chunk);
      return new_chunk->allocate_new(size);
   }
   
   inline void deallocate(void* ptr)
   {
      add_free_queue(free, ptr);
   }

   static inline size_t num_elems_chunk(const size_t size) { return size < 128 ? 64 : 16; }

   inline void init(const size_t _size)
   {
      size = _size;
      first_chunk = nullptr;
      new_chunk = nullptr;
      init_free_queue(free);
      num_elems_per_chunk = num_elems_chunk(_size);
   }

   explicit chunkgroup()
   {
   }
   
   explicit chunkgroup(const size_t _size):
      size(_size), first_chunk(nullptr),
      new_chunk(nullptr),
      num_elems_per_chunk(num_elems_chunk(_size))
   {
      assert(_size >= sizeof(mem_node));
   }
   
   ~chunkgroup(void)
   {
      chunk *cur(first_chunk);
      
      while (cur) {
         chunk *next(cur->next_chunk);
         chunk::destroy(cur);
         cur = next;
      }
   }
};

}

#endif

