
#ifndef MEM_CHUNK_HPP
#define MEM_CHUNK_HPP

#include "mem/stat.hpp"
#include "utils/types.hpp"

namespace mem
{
   
class chunkgroup;

class chunk
{
private:
   
   friend class chunkgroup;

   chunk *next_chunk;
   
   utils::byte *cur;
   utils::byte *top;

public:
   
   inline void* allocate_new(const size_t size)
   {
      unsigned char *old_cur(cur);

      cur += size;
      
      if(cur > top)
         return nullptr; // full
      
      return old_cur;
   }

   static inline chunk* create(const size_t size, const size_t num_elems, chunk *next) {
      const size_t total(sizeof(chunk) + size * num_elems);
      utils::byte *p(new utils::byte[total]);
      register_malloc(total);
      chunk *c((chunk*)p);
      c->cur = p + sizeof(chunk);
      c->top = p + total;
      c->next_chunk = next;
      return c;
   }

   static inline void destroy(chunk *c) {
      utils::byte *p((utils::byte*)c);
      const size_t total(c->top - p);
      (void)total;

      delete []p;
   }
};

}

#endif
