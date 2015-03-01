
#ifndef UTIL_CSV_LINE_HPP
#define UTIL_CSV_LINE_HPP

#include <ostream>
#include <vector>
#include <string>

#include "utils/utils.hpp"

namespace utils
{

class csv_line
{
private:
   
   std::vector<std::string> data;
   bool is_header;
   
public:
   
   void print(std::ostream& out) const
   {
      if(is_header)
         out << "# ";
      for(auto it(data.begin()); it != data.end(); ++it) {
         if(it != data.begin())
            out << " ";
         out << *it;
      }
      out << std::endl;
   }
   
   void reset(void)
   {
      data.clear();
      is_header = false;
   }
   
   void set_header(void)
   {
      is_header = true;
   }
   
   void add(const std::string& str)
   {
      data.push_back(str);
   }
   
   csv_line&
   operator<<(const std::string& str)
   {
      add(str);
      return *this;
   }

   template <class T>
   csv_line& operator<<(const T& x)
   {
      add(utils::to_string(x));
      return *this;
   }
   
   explicit csv_line(void): is_header(false) {}
   
   virtual ~csv_line(void) {}
};

}

#endif
