
#ifndef UTILS_UTILS_HPP
#define UTILS_UTILS_HPP

#include <sstream>
#include <vector>
#include <string>

#include "utils/types.hpp"
#include "utils/random.hpp"

namespace utils {

#define true_likely(x) __builtin_expect(!!(x), 1)
#define false_likely(x) __builtin_expect(!!(x), 0)
#define cmpxchg(P, O, N) __sync_val_compare_and_swap((P), (O), (N))
#if defined(__x86_64__) || defined(_M_X64)
#define BITMAP_TYPE uint64_t
#elif defined(__i386) || defined(_M_IX86)
#define BITMAP_TYPE uint32_t
#else
#error "could not deduce architecture"
#endif
#define BITMAP_BITS (sizeof(BITMAP_TYPE) * 8)

void set_random_generator(randgen *);
size_t number_cpus(void);

template <typename T>
std::string to_string(const T& obj) {
   std::stringstream ss;
   ss << obj;
   return ss.str();
}

template <class T>
bool from_string(T& t, const std::string& s,
                 std::ios_base& (*f)(std::ios_base&)) {
   std::istringstream iss(s);
   return !(iss >> f >> t).fail();
}

template <typename T>
T upper_log2(const T n) {
   T i(0);

   if (n > 0) {
      T m = 1;
      while (1) {
         if (m >= n) return i;
         m <<= 1;
         i++;
      }
   } else
      return -1;
}

template <typename T>
T next_power2(const T val) {
   T ret(1);

   while (ret < val) ret <<= 1;

   return ret;
}

template <typename T>
T power(const T base, const T exp) {
   assert(exp >= 0);

   if (exp == 0) return 1;

   T ret(base);

   for (size_t i = 1; i < (size_t)exp; ++i) ret *= base;

   return ret;
}

size_t random_unsigned(const size_t);

void write_strings(const std::vector<std::string>&, std::ostream&,
                   const size_t);

inline size_t next_multiple_of_uint(const size_t v) {
   size_t ret(v / BITMAP_BITS);
   if (v % BITMAP_BITS > 0) ret++;
   return ret;
}

static inline bool is_prime(std::size_t n) {
   //check if n is a multiple of 2
   if (n%2==0) return false;
   //if not, then just check the odds
   for(std::size_t i=3;i*i<=n;i+=2) {
      if(n%i==0)
         return false;
   }
   return true;
}

static inline std::size_t next_prime(std::size_t x) {
   switch (x) {
      case 0:
      case 1:
      case 2:
         return 2;
      case 3:
         return 3;
      case 4:
      case 5:
         return 5;
   }
   std::size_t k = x / 6;
   std::size_t i = x - 6 * k;
   std::size_t o = i < 2 ? 1 : 5;
   x = 6 * k + o;
   for (i = (3 + o) / 2; !is_prime(x); x += i) i ^= 6;
   return x;
}

static inline std::size_t previous_prime(std::size_t x) {
   for(; !is_prime(x) && x > 0; --x) {
   }
   return x;
}

static inline uint64_t mod_hash(const uint64_t hsh, const uint64_t size) {
   // size must be a power of 2
   return hsh & (size - 1);
}

}

#endif
