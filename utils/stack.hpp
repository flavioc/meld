
#ifndef UTILS_STACK_HPP
#define UTILS_STACK_HPP

#include <stack>
#include <vector>

#include "mem/allocator.hpp"
#include "utils/utils.hpp"

namespace utils
{

/* this is a vector-based stack that allows the internal vector to reserve memory before hand */
template <typename T>
class stack: public std::stack<T, std::vector<T, mem::allocator<T> > >
{
public:
   
   typedef std::vector<T, mem::allocator<T> > container_type;
   
   inline void reserve(const size_t defsize)
   {
      this->c.reserve(next_power2(defsize));
   }
   
   explicit stack(const size_t defsize, container_type vec = container_type()):
      std::stack<T, container_type>(vec)
   {
      reserve(defsize);
   }
   
   explicit stack(void)
   {
   }
};

}

#endif
