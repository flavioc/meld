
#ifndef VM_BITMAP_HPP
#define VM_BITMAP_HPP

#include "vm/defs.hpp"
#include "utils/utils.hpp"
#include "mem/allocator.hpp"
#include <iostream>

namespace vm {

struct bitmap {
   static_assert(sizeof(BITMAP_TYPE) == sizeof(BITMAP_TYPE*),
                 "Pointer and bitmap type must have the same size");

   BITMAP_TYPE first;
   BITMAP_TYPE* rest;
#ifdef EXTRA_ASSERTS
   bool initialized = false;
#endif

   public:
   inline void clear(const size_t size) {
#ifdef EXTRA_ASSERTS
      initialized = true;
#endif
      first = 0;
      if (false_likely(size > 1))
         memset(rest, 0, sizeof(BITMAP_TYPE) * (size - 1));
   }

   class iterator {
  private:
      BITMAP_TYPE* rest;
      bitmap* b;
      bool in_first;
      size_t pos;
      size_t total;
      size_t items;

      inline bool move_position(void) {
         pos++;
         total++;
         if (total == items) {
            return false;
         }
         if (pos == BITMAP_BITS) {
            pos = 0;
            if (in_first) {
               in_first = false;
               rest = b->rest;
            } else {
               rest++;
            }
         }
         return true;
      }

      inline void find_first_pos(void) {
         while (!(*rest & ((BITMAP_TYPE)0x1 << (BITMAP_TYPE)pos))) {
            if (!move_position()) return;
         }
      }

  public:
      inline iterator& operator++(void) {
         if (move_position()) find_first_pos();
         return *this;
      }

      inline iterator operator++(int) {
         if (move_position()) find_first_pos();
         return *this;
      }

      inline size_t operator*(void) const { return total; }

      inline bool end(void) const { return total == items; }

      iterator(bitmap* _b, const size_t _items)
          : rest(&(_b->first)),
            b(_b),
            in_first(true),
            pos(0),
            total(0),
            items(_items) {
         if (_b->rest == nullptr && _b->first == (BITMAP_TYPE)0)
            total = items;  // the end...
         else
            find_first_pos();
      }
   };

   inline iterator begin(const size_t items) { return iterator(this, items); }

   inline bool empty(const size_t size) const {
      if (true_likely(size == 1))
         return first == (BITMAP_TYPE)0;
      else {
         if (first != (BITMAP_TYPE)0) return false;
         for (size_t i(0); i < size - 1; ++i) {
            if (*(rest + i) != (BITMAP_TYPE)0) return false;
         }
         return true;
      }
   }

#define BITMAP_GET_BIT(ARR, POS) \
   ((ARR) & ((BITMAP_TYPE)0x1 << (BITMAP_TYPE)(POS)))

   inline size_t front(const size_t size) {
      if (true_likely(size == 1)) {
         if (first != (BITMAP_TYPE)0) return ffsl(first) - 1;
         return -1;
      } else {
         if (first != (BITMAP_TYPE)0)
            return ffsl(first) - 1;
         else {
            int pos = BITMAP_BITS;
            // look into others
            for (size_t j(0); j < size - 1; ++j) {
               BITMAP_TYPE a(*(rest + j));
               if (a != (BITMAP_TYPE)0)
                  return pos + ffsl(a) - 1;
               else
                  pos += BITMAP_BITS;
            }
            return -1;
         }
      }
   }

   inline void unset_front(const size_t size) {
      if (true_likely(size == 1)) {
         assert(first != (BITMAP_TYPE)0);
         first = first & (first - (BITMAP_TYPE)1);
      } else {
         if (first != (BITMAP_TYPE)0)
            first = first & (first - (BITMAP_TYPE)1);
         else {
            for (size_t j(0); j < size - 1; ++j) {
               BITMAP_TYPE a(*(rest + j));
               if (a != (BITMAP_TYPE)0) {
                  *(rest + j) = a & (a - (BITMAP_TYPE)1);
                  return;
               }
            }
         }
      }
   }

   inline size_t remove_front(const size_t size) {
      if (true_likely(size == 1)) {
         assert(first != (BITMAP_TYPE)0);
         const size_t ret(ffsl(first) - 1);
         first = first & (first - (BITMAP_TYPE)1);
         return ret;
      } else {
         if (first != (BITMAP_TYPE)0) {
            const size_t ret(ffsl(first) - 1);
            first = first & (first - (BITMAP_TYPE)1);
            return ret;
         } else {
            size_t pos(BITMAP_BITS);
            for (size_t j(0); j < size - 1; ++j, pos += BITMAP_BITS) {
               BITMAP_TYPE a(*(rest + j));
               if (a != (BITMAP_TYPE)0) {
                  pos += ffsl(a) - 1;
                  *(rest + j) = a & (a - (BITMAP_TYPE)1);
                  return pos;
               }
            }
            return -1;
         }
      }
   }

   // remove all bits in our bitmap set in the other bitmap
   inline void unset_bits(bitmap& other, const size_t size) {
      // we assume that both have same size!
      if (true_likely(size == 1))
         first = first & ~(other.first);
      else {
         first = first & ~(other.first);
         for (size_t i(0); i < size - 1; ++i)
            *(rest + i) = *(rest + i) & ~(*(other.rest + i));
      }
   }

   // set the bitmap as the result of OR'ing with the other argument
   inline void set_bits_or(const bitmap& other, const size_t size) {
      first = first | other.first;
      for (size_t i(0); i < size - 1; ++i)
         *(rest + i) = *(rest + i) | *(other.rest + i);
   }

   // and the two arguments and set the corresponding bits on our bitmap
   inline void set_bits_of_and_result(const bitmap& a, const bitmap& b,
                                      const size_t size) {
      first = (a.first & b.first);
      for (size_t i(0); i < size - 1; ++i)
         *(rest + i) = (*(a.rest + i) & *(b.rest + i));
   }

   inline void unset_bits_of_and_result(const bitmap& a, const bitmap& b,
                                        const size_t size) {
      first = first & ~(a.first & b.first);
      for (size_t i(0); i < size - 1; ++i)
         *(rest + i) = *(rest + i) & ~(*(a.rest + i) & *(b.rest + i));
   }

   // set bit to true
   inline void set_bit(const size_t i) {
#ifdef EXTRA_ASSERTS
      assert(initialized);
#endif
      BITMAP_TYPE* p(get_array(i));

      *p = *p | ((BITMAP_TYPE)0x1 << (BITMAP_TYPE)(i % BITMAP_BITS));
   }

   inline void unset_bit(const size_t i) {
      BITMAP_TYPE* p(get_array(i));

      *p = *p &
           (~(BITMAP_TYPE)((BITMAP_TYPE)0x1 << (BITMAP_TYPE)(i % BITMAP_BITS)));
   }

   inline BITMAP_TYPE* get_array(const size_t i) {
      const size_t id(i / BITMAP_BITS);

      if (id == 0) return &first;
      return rest + (id - 1);
   }

   inline BITMAP_TYPE const* get_array(const size_t i) const {
      const size_t id(i / BITMAP_BITS);

      if (id == 0) return (const BITMAP_TYPE*)&first;
      return rest + (id - 1);
   }

   inline bool get_bit(const size_t i) const {
#ifdef EXTRA_ASSERTS
      assert(initialized);
#endif
      const BITMAP_TYPE* p(get_array(i));

      return BITMAP_GET_BIT(*p, i % BITMAP_BITS);
   }

   static inline void create(bitmap& b, const size_t size) {
      if (size >= 2)
         b.rest = mem::allocator<BITMAP_TYPE>().allocate(size - 1);
      else
         b.rest = nullptr;
      b.clear(size);
   }

   static inline void destroy(bitmap& b, const size_t size) {
      if (size >= 2) mem::allocator<BITMAP_TYPE>().deallocate(b.rest, size - 1);
   }
};
}

#endif
