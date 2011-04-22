
#ifndef MEM_CHUNK_HPP
#define MEM_CHUNK_HPP

namespace mem
{

class chunk
{
private:
   
   static const size_t NUM_ELEMS = 64;
   
   unsigned char *cur;
   unsigned char *bottom;
   unsigned char *top;
   
public:
   
   inline void* allocate(const size_t size)
   {
      unsigned char *old_cur(cur);

      cur += size;
      
      if(cur > top)
         return NULL; // full
      
      return old_cur;
   }
   
   explicit chunk(const size_t size)
   {
      const size_t total(size * NUM_ELEMS);
      //printf("Asked for %d bytes of mem\n", total); 
      
      bottom = new unsigned char[total];
      cur = bottom;
      top = bottom + total;
   }
   
   ~chunk(void)
   {
      delete []bottom;
   }
};

}

#endif
