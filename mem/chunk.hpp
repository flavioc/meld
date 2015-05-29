
#ifndef MEM_CHUNK_HPP
#define MEM_CHUNK_HPP

#include "mem/stat.hpp"
#include "utils/types.hpp"

namespace mem
{
   
struct chunkgroup;

struct chunk
{
private:
   
   friend struct chunkgroup;

   chunk *next_chunk;
   chunk *prev_chunk;
   
   utils::byte *cur;
   utils::byte *top;

public:

   inline void set_prev(chunk *c)
   {
      prev_chunk = c;
   }
   
   inline void* allocate_new(const size_t size)
   {
      utils::byte *old_cur(cur);

      cur += size + MEM_ADD_OBJS;
      
      if(cur > top)
         return nullptr; // full
      
      return (old_cur + MEM_ADD_OBJS);
   }

   static inline chunk* create(const size_t size, const size_t num_elems, chunk *next) {
      const size_t total(sizeof(chunk) + (size + MEM_ADD_OBJS) * num_elems);
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
