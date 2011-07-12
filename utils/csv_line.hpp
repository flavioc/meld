
#ifndef UTIL_CSV_LINE_HPP
#define UTIL_CSV_LINE_HPP

#include <ostream>
#include <vector>
#include <string>

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
      for(std::vector<std::string>::const_iterator it(data.begin()); it != data.end(); ++it) {
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
   
   explicit csv_line(void): is_header(false) {}
   
   virtual ~csv_line(void) {}
};

}

#endif
