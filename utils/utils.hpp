
#ifndef UTILS_UTILS_HPP
#define UTILS_UTILS_HPP

#include <sstream>
#include <vector>
#include <string>

namespace utils
{

#define true_likely(x)      __builtin_expect(!!(x), 1)
#define false_likely(x)    __builtin_expect(!!(x), 0)
#define cmpxchg(P, O, N) __sync_val_compare_and_swap((P), (O), (N))
#define BITMAP_TYPE uint64_t
#define BITMAP_BITS (sizeof(BITMAP_TYPE) * 8)

size_t number_cpus(void);

template <typename T>
std::string to_string(const T& obj)
{
	std::stringstream ss;
	ss << obj;
	return ss.str();
}

template <class T>
bool from_string(T& t, 
		const std::string& s, 
		std::ios_base& (*f)(std::ios_base&))
{
	std::istringstream iss(s);
	return !(iss >> f >> t).fail();
}

template <typename T>
T upper_log2(const T n)
{
   T i(0);
   
   if (n > 0) 
   { 
      T m = 1; 
      while(1) 
      { 
         if (m >= n) 
            return i; 
         m <<= 1; 
         i++; 
      } 
   } 
   else  
      return -1;
}

template <typename T>
T next_power2(const T val)
{
   T ret(1);
   
   while(ret < val)
      ret <<= 1;
      
   return ret;
}

template <typename T>
T power(const T base, const T exp)
{
   assert(exp >= 0);
   
   if(exp == 0) return 1;
   
   T ret(base);
   
   for(size_t i = 1; i < (size_t)exp; ++i)
      ret *= base;
      
   return ret;
}

size_t random_unsigned(const size_t);

void write_strings(const std::vector<std::string>&, std::ostream&, const size_t);

inline size_t next_multiple_of_uint(const size_t v)
{
   size_t ret(v / BITMAP_BITS);
   if(v % BITMAP_BITS > 0)
      ret++;
   return ret;
}

static inline bool
is_prime(std::size_t x)
{
   std::size_t o = 4;
   for (std::size_t i = 5; true; i += o)
   {
      std::size_t q = x / i;
      if (q < i)
         return true;
      if (x == q * i)
         return false;
      o ^= 6;
   }
   return true;
}

static inline std::size_t
next_prime(std::size_t x)
{
   switch (x)
   {
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
   for (i = (3 + o) / 2; !is_prime(x); x += i)
      i ^= 6;
   return x;
}

}

#endif
