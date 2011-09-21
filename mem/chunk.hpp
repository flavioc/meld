
#ifndef MEM_CHUNK_HPP
#define MEM_CHUNK_HPP

#include "mem/stat.hpp"

namespace mem
{
   
class chunkgroup;

class chunk
{
private:
   
   friend class chunkgroup;
   
   chunk *next_chunk;
   
   unsigned char *cur;
   unsigned char *bottom;
   unsigned char *top;
   
public:
   
   inline void set_next(chunk *ptr)
   {
      next_chunk = ptr;
   }
   
   inline void* allocate(const size_t size)
   {
      unsigned char *old_cur(cur);

      cur += size;
      
      if(cur > top)
         return NULL; // full
      
      return old_cur;
   }
   
   explicit chunk(const size_t size, const size_t num_elems):
      next_chunk(NULL)
   {
      const size_t total(size * num_elems);
      
      bottom = new unsigned char[total];
      cur = bottom;
      top = bottom + total;
      
      register_malloc();
   }
   
   ~chunk(void)
   {
      delete []bottom;
   }
};

}

#endif
