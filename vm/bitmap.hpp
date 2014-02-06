
#ifndef VM_BITMAP_HPP
#define VM_BITMAP_HPP

#include "vm/defs.hpp"
#include "utils/utils.hpp"
#include "mem/allocator.hpp"
#include "vm/all.hpp"
#include <iostream>

namespace vm
{

struct bitmap {

   class iterator
   {
      private:

         BITMAP_TYPE *a;
         size_t pos;
         size_t total;
         BITMAP_TYPE *last;
         size_t items;

         inline bool move_position(void)
         {
            pos++;
            total++;
            if(total == items) {
               a = last;
               return false;
            }
            if(pos == BITMAP_BITS) {
               pos = 0;
               a++;
               if(a == last)
                  return false;
            }
            return true;
         }

         inline void find_first_pos(void)
         {
            while(!(*a & ((BITMAP_TYPE)0x1 << (BITMAP_TYPE)pos))) {
               if(!move_position())
                  return;
            }
         }

      public:

         inline iterator& operator++(void)
         {
            if(move_position())
               find_first_pos();
            return *this;
         }

         inline iterator operator++(int)
         {
            if(move_position())
               find_first_pos();
            return *this;
         }

         inline size_t operator*(void) const
         {
            return total;
         }

         inline bool end(void) const
         {
            return a == last;
         }

         iterator(BITMAP_TYPE *_a, BITMAP_TYPE *_last, const size_t _items):
            a(_a), pos(0), total(0), last(_last), items(_items)
         {
            find_first_pos();
         }
   };

   inline iterator begin(const size_t size, const size_t items) const { return iterator((BITMAP_TYPE*)this, ((BITMAP_TYPE*)this) + size, items); }
   inline iterator begin(const size_t size, const size_t items) { return iterator((BITMAP_TYPE*)this, ((BITMAP_TYPE*)this) + size, items); }

   inline void clear(const size_t size)
   {
      memset(this, 0, sizeof(BITMAP_TYPE) * size);
   }

   // set bit to true
   inline void set_bit(const size_t i)
   {
      BITMAP_TYPE *a((BITMAP_TYPE*)this);
      BITMAP_TYPE *p(a + (i / BITMAP_BITS));

      *p = *p | ((BITMAP_TYPE)0x1 << (BITMAP_TYPE)(i % BITMAP_BITS));
   }

   inline void unset_bit(const size_t i)
   {
      BITMAP_TYPE *a((BITMAP_TYPE*)this);
      BITMAP_TYPE *p(a + (i / BITMAP_BITS));

      *p = *p & (~(BITMAP_TYPE)((BITMAP_TYPE)0x1 << (BITMAP_TYPE)(i % BITMAP_BITS)));
   }

   inline bool get_bit(const size_t i)
   {
      BITMAP_TYPE *a((BITMAP_TYPE*)this);
      BITMAP_TYPE *p(a + (i / BITMAP_BITS));

      return *p & ((BITMAP_TYPE)0x1 << (BITMAP_TYPE)(i % BITMAP_BITS));
   }

   static inline bitmap*
   create(const size_t size)
   {
      return (bitmap*)mem::allocator<BITMAP_TYPE>().allocate(size);
   }

   static inline void
   destroy(bitmap *b, const size_t size)
   {
      mem::allocator<BITMAP_TYPE>().deallocate((BITMAP_TYPE*)b, size);
   }
};


}

#endif

