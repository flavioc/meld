
#ifndef UTILS_INTERVAL_HPP
#define UTILS_INTERVAL_HPP

#include <ostream>

namespace utils
{
   
template <class T>
class interval
{
private:
   
   T min_val;
   T max_val;
   
public:
   
   const bool between(const T& val) const
   {
      return val >= min_val && val <= max_val;
   }
   
   void update(const T& val)
   {
      if(val < min_val)
         min_val = val;
      else if(val > max_val)
         max_val = val;
   }
   
   void print(std::ostream& cout) const
   {
      cout << "<" << min_val << ", " << max_val << ">";
   }
   
   explicit interval(const T& _min_val, const T& _max_val):
      min_val(_min_val), max_val(_max_val)
   {}
};

template <typename T>
std::ostream& operator<<(std::ostream& cout, const interval<T>& inter)
{
   inter.print(cout);
   return cout;
}

}

#endif
