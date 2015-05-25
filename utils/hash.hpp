
#ifndef UTILS_HASH_HPP
#define UTILS_HASH_HPP

#include <cstring>
#include <cmath>

#include "utils/types.hpp"

namespace utils
{

template <typename T>
struct pointer_hash {
   size_t operator()(const T* val) const {
      static const size_t shift((size_t)std::log2(1 + sizeof(T)));
      return (size_t)(val) >> shift;
   }
};

static inline uint64_t fnv1_hash(::utils::byte* key, const size_t n_bytes) {
   ::utils::byte* p = key;
   uint64_t h = 14695981039346656037ul;
   for (size_t i = 0; i < n_bytes; i++) h = (h * 1099511628211) ^ p[i];
   return h;
}

template <typename T>
struct fnv1_hasher {
   size_t operator()(const T val) const {
      return fnv1_hash((::utils::byte*)&val, sizeof(T));
   }
};

}

#endif
