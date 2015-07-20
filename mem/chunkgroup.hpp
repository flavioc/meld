
#ifndef MEM_CHUNKGROUP_HPP
#define MEM_CHUNKGROUP_HPP

#include <limits>
#include <assert.h>

#include "mem/mem_node.hpp"
#include "mem/bigchunk.hpp"

namespace mem {

struct chunkgroup {
   public:
   // next pointer used by mem/pool.
   chunkgroup *next;
   chunkgroup *twice;
   size_t size;

   private:

   utils::byte *ptr, *end;

   size_t num_elems_per_chunk;

   free_queue free;

   public:

   inline bool has_free() {
      return !free_queue_empty(free);
   }

   inline void* allocate_free() {
      return free_queue_pop(free);
   }

   inline void *allocate(bigchunk *bc) {
      assert(ptr <= end);

      if (ptr + size > end) {
         // needs another chunk
         if (num_elems_per_chunk < std::numeric_limits<std::size_t>::max() / 2)
            num_elems_per_chunk += num_elems_per_chunk/2;  // increase number of elements

         auto p(bc->fetch(num_elems_per_chunk * size, size));
         ptr = p.first;
         end = p.second;
         assert(end > ptr);
         assert((size_t)(end - ptr) >= size);
      }

      auto oldp(ptr);
      ptr = ptr + size;
      assert(ptr <= end);
      return oldp;
   }

   inline void deallocate(void *ptr) { add_free_queue(free, ptr); }

   inline void set_twice(chunkgroup *t) { twice = t; }
   inline chunkgroup *get_twice() const { return twice; }

   static inline size_t num_elems_chunk(const size_t size) {
      return size < 128 ? 32 : 16;
   }

   inline void init(const size_t _size, bigchunk *bc) {
      twice = nullptr;
      size = _size;
      init_free_queue(free);
      num_elems_per_chunk = num_elems_chunk(_size);
      auto p(bc->fetch(num_elems_per_chunk * size, size));
      ptr = p.first;
      end = p.second;
      assert(end > ptr);
   }
};
}

#endif
