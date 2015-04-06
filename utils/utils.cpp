
#include <iostream>
#include <fstream>
#include <random>
#include <unistd.h>
#include <assert.h>

#include "utils/utils.hpp"
#include "utils/random.hpp"

using namespace std;

namespace utils
{
   
__thread randgen *gen{nullptr};

void
set_random_generator(randgen *_gen)
{
   gen = _gen;
}

size_t
number_cpus(void)
{
	  return (size_t)sysconf(_SC_NPROCESSORS_ONLN);
}

size_t
random_unsigned(const size_t lim)
{
   assert(gen);
   return gen->operator()(lim);
}

void
write_strings(const vector<string>& v, ostream& out, const size_t tab)
{
   static const int SIZE_LINE(75);
   int left_to_write(SIZE_LINE);
   bool starting = true;

   for(const auto & elem : v) {
      if(starting) {
         for(size_t i(0); i < tab; ++i)
            out << "\t";
         starting = false;
      }
      out << elem;
      left_to_write -= elem.length();
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
