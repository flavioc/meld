
#ifndef VM_BITMAP_HPP
#define VM_BITMAP_HPP

#include <boost/static_assert.hpp>

#include "vm/defs.hpp"
#include "utils/utils.hpp"
#include "mem/allocator.hpp"
#include "vm/all.hpp"
#include <iostream>

namespace vm
{

struct bitmap {

   //BOOST_STATIC_ASSERT(sizeof(BITMAP_TYPE) == sizeof(BITMAP_TYPE*));

   BITMAP_TYPE first;
   BITMAP_TYPE *rest;

   class iterator
   {
      private:

         BITMAP_TYPE *rest;
         bitmap *b;
         bool in_first;
         size_t pos;
         size_t total;
         size_t items;

         inline bool move_position(void)
         {
            pos++;
            total++;
            if(total == items) {
               return false;
            }
            if(pos == BITMAP_BITS) {
               pos = 0;
               if(in_first) {
                  in_first = false;
                  rest = b->rest;
               } else {
                  rest++;
               }
            }
            return true;
         }

         inline void find_first_pos(void)
         {
            while(!(*rest & ((BITMAP_TYPE)0x1 << (BITMAP_TYPE)pos))) {
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
            return total == items;
         }

         iterator(bitmap *_b, const size_t _items):
            rest(&(_b->first)), b(_b), in_first(true), pos(0), total(0), items(_items)
         {
            if(_b->rest == NULL && _b->first == (BITMAP_TYPE)0)
               total = items; // the end...
            else
               find_first_pos();
         }
   };

   inline iterator begin(const size_t items) { return iterator(this, items); }

   inline void clear(const size_t size)
   {
      first = 0;
      if(size > 1)
         memset(rest, 0, sizeof(BITMAP_TYPE) * (size-1));
   }

   // set bit to true
   inline void set_bit(const size_t i)
   {
      BITMAP_TYPE *p(get_array(i));

      *p = *p | ((BITMAP_TYPE)0x1 << (BITMAP_TYPE)(i % BITMAP_BITS));
   }

   inline void unset_bit(const size_t i)
   {
      BITMAP_TYPE *p(get_array(i));

      *p = *p & (~(BITMAP_TYPE)((BITMAP_TYPE)0x1 << (BITMAP_TYPE)(i % BITMAP_BITS)));
   }

   inline BITMAP_TYPE *get_array(const size_t i)
   {
      const size_t id(i / BITMAP_BITS);

      if(id == 0)
         return &first;
      return rest + (id - 1);
   }

   inline BITMAP_TYPE const *get_array(const size_t i) const
   {
      const size_t id(i / BITMAP_BITS);

      if(id == 0)
         return (const BITMAP_TYPE*)&first;
      return rest + (id - 1);
   }

   inline bool get_bit(const size_t i) const
   {
      const BITMAP_TYPE *p(get_array(i));

      return *p & ((BITMAP_TYPE)0x1 << (BITMAP_TYPE)(i % BITMAP_BITS));
   }

   static inline void
   create(bitmap& b, const size_t size)
   {
      if(size >= 2)
         b.rest = mem::allocator<BITMAP_TYPE>().allocate(size-1);
      else
         b.rest = NULL;
   }

   static inline void
   destroy(bitmap& b, const size_t size)
   {
      if(size >= 2)
         mem::allocator<BITMAP_TYPE>().deallocate(b.rest, size-1);
   }
};


}

#endif

