
#ifndef MEM_ALLOCATOR_HPP
#define MEM_ALLOCATOR_HPP

#include <limits>
#include <tr1/unordered_set>

#include "conf.hpp"
#include "mem/center.hpp"
#include "mem/thread.hpp"
#include "mem/stat.hpp"
#include <iostream>

namespace mem
{

template <class T>
class allocator
{
public:
   
   typedef T value_type;
   typedef value_type* pointer;
   typedef const value_type* const_pointer;
   typedef value_type& reference;
   typedef const value_type& const_reference;
   typedef std::size_t size_type;
   typedef std::ptrdiff_t difference_type;
   
public:
   
   template <typename U>
   struct rebind {
      typedef allocator<U> other;
   };
   
public:
   
   inline explicit allocator(void) {}
   inline ~allocator(void) {}

	inline allocator(allocator const&) {}
   
   template <typename U>
   inline allocator(allocator<U> const&) {}
   
   inline pointer address(reference r) { return &r; }
   inline const_pointer address(const_reference r) { return &r; }
   
   inline pointer allocate(size_type cnt,
      typename std::allocator<void>::const_pointer = 0)
   {
      return reinterpret_cast<pointer>(mem::center::allocate(cnt, sizeof(T)));
   }
   
   inline void deallocate(pointer p, size_type cnt)
   {
      mem::center::deallocate(p, cnt, sizeof(T));
   }
   
   inline size_type max_size() const {
      return std::numeric_limits<size_type>::max() / sizeof(T);
   }
   
   inline void construct(pointer p, const T& t) { new (p) T(t); }
   inline void destroy(pointer p) { p->~T(); }
   
   inline bool operator==(allocator const&) { return true; }
   inline bool operator!=(allocator const& a) { return !operator==(a); }
};

}

#endif
