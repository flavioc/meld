
#include <fstream>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>

#include "utils.hpp"

using namespace std;

namespace utils
{
   
static boost::mt19937 gen(time(NULL));

size_t
number_cpus(void)
{
	  return (size_t)sysconf(_SC_NPROCESSORS_ONLN);
}

size_t
random_unsigned(const size_t lim)
{
   boost::uniform_int<> dist(0, lim-1);
   boost::variate_generator<boost::mt19937&, boost::uniform_int<> > die(gen, dist);
   return (size_t)die();
}

void
write_strings(const vector<string>& v, ostream& out, const size_t tab)
{
   static const int SIZE_LINE(75);
   int left_to_write(SIZE_LINE);
   bool starting = true;

   for(vector<string>::const_iterator it(v.begin()), end(v.end()); it != end; ++it) {
      if(starting) {
         for(size_t i(0); i < tab; ++i)
            out << "\t";
         starting = false;
      }
      out << *it;
      left_to_write -= it->length();
      if(left_to_write <= 0) {
         left_to_write = SIZE_LINE;
         starting = true;
         out << endl;
      } else
         out << " ";
   }

   if(left_to_write > 0 && !starting) {
      out << endl;
   }
}

}
