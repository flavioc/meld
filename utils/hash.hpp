
#ifndef UTILS_HASH_HPP
#define UTILS_HASH_HPP

#include <cstring>
#include <cmath>

namespace utils
{

template <typename T>
struct pointer_hash {
   size_t operator()(const T* val) const {
      static const size_t shift((size_t)std::log2(1 + sizeof(T)));
      return (size_t)(val) >> shift;
   }
};

}

#endif
