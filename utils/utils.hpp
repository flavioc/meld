
#ifndef UTILS_HPP
#define UTILS_HPP

#include <sstream>

namespace utils
{

const size_t number_cpus(void);

template <typename T>
std::string number_to_string (T number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

}

#endif
