
#ifndef VM_BITMAP_STATIC_HPP
#define VM_BITMAP_STATIC_HPP

#include <iostream>

#include "vm/defs.hpp"
#include "utils/utils.hpp"
#include "mem/allocator.hpp"

namespace vm {

template <size_t SIZE>
struct bitmap_static {
   BITMAP_TYPE data[SIZE] = {};

   class iterator {
  private:
      BITMAP_TYPE* data;
      size_t pos;
      size_t total;
      size_t items;

      inline bool move_position(void) {
         pos++;
         total++;
         if (total == items)
            return false;
         if (pos == BITMAP_BITS) {
            pos = 0;
            data++;
         }
         return true;
      }

      inline void find_first_pos(void) {
         while (!(*data & ((BITMAP_TYPE)0x1 << (BITMAP_TYPE)pos))) {
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

      iterator(bitmap_static* _b, const size_t _items)
          : data(_b->data),
            pos(0),
            total(0),
            items(_items) {
            find_first_pos();
      }
   };

   inline iterator begin(const size_t items) { return iterator(this, items); }

   inline bool empty(const size_t inv = 0) const {
      (void)inv;
      for (size_t i(0); i < SIZE; ++i) {
         if (*(data + i) != (BITMAP_TYPE)0) return false;
      }
      return true;
   }

   inline void clear() {
      memset(data, 0, sizeof(BITMAP_TYPE) * SIZE);
   }

#define BITMAP_GET_BIT(ARR, POS) \
   ((ARR) & ((BITMAP_TYPE)0x1 << (BITMAP_TYPE)(POS)))

   inline size_t front() {
      size_t pos = 0;
      // look into others
      for (size_t j(0); j < SIZE; ++j) {
         BITMAP_TYPE a(*(data + j));
         if (a != (BITMAP_TYPE)0)
            return pos + ffsl(a) - 1;
         else
            pos += BITMAP_BITS;
      }
      return -1;
   }

   inline void unset_front() {
      for (size_t j(0); j < SIZE; ++j) {
         BITMAP_TYPE a(*(data + j));
         if (a != (BITMAP_TYPE)0) {
            *(data + j) = a & (a - (BITMAP_TYPE)1);
            return;
         }
      }
   }

   inline size_t remove_front(const size_t inv = 0) {
      (void)inv;
      size_t pos(0);
      for (size_t j(0); j < SIZE; ++j, pos += BITMAP_BITS) {
         BITMAP_TYPE a(*(data + j));
         if (a != (BITMAP_TYPE)0) {
            pos += ffsl(a) - 1;
            *(data + j) = a & (a - (BITMAP_TYPE)1);
            return pos;
         }
      }
      return -1;
   }

   // remove all bits in our bitmap set in the other bitmap
   inline void unset_bits(bitmap_static& other) {
      for (size_t i(0); i < SIZE; ++i)
         *(data + i) = *(data + i) & ~(*(other.data + i));
   }

   // set the bitmap as the result of OR'ing with the other argument
   inline void set_bits_or(const bitmap_static& other) {
      for (size_t i(0); i < SIZE; ++i)
         *(data + i) = *(data + i) | *(other.data + i);
   }

   // and the two arguments and set the corresponding bits on our bitmap
   inline void set_bits_of_and_result(const bitmap_static& a, const bitmap_static& b) {
      for (size_t i(0); i < SIZE; ++i)
         *(data + i) = (*(a.data + i) & *(b.data + i));
   }

   inline void unset_bits_of_and_result(const bitmap_static& a, const bitmap_static& b) {
      for (size_t i(0); i < SIZE; ++i)
         *(data + i) = *(data + i) & ~(*(a.data + i) & *(b.data + i));
   }

   // set bit to true
   inline void set_bit(const size_t i) {
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
      return data + id;
   }

   inline BITMAP_TYPE const* get_array(const size_t i) const {
      const size_t id(i / BITMAP_BITS);
      return data + id;
   }

   inline bool get_bit(const size_t i) const {
      const BITMAP_TYPE* p(get_array(i));

      return BITMAP_GET_BIT(*p, i % BITMAP_BITS);
   }
};
}

#endif
