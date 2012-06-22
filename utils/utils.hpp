
#ifndef UTILS_UTILS_HPP
#define UTILS_UTILS_HPP

#include <sstream>

namespace utils
{

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

}

#endif
