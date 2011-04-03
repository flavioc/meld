
#include "utils.hpp"

namespace utils
{

const size_t
number_cpus(void)
{
	  return (size_t)sysconf(_SC_NPROCESSORS_ONLN);
}

}
