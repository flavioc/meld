
#ifndef UTILS_UTILS_HPP
#define UTILS_UTILS_HPP

#include <sstream>

namespace utils
{

const size_t number_cpus(void);

template <typename T>
std::string to_string(const T& obj)
{
	std::stringstream ss;
	ss << obj;
	return ss.str();
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

const size_t random_unsigned(const size_t);

}

#endif
