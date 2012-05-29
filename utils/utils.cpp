
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

}
