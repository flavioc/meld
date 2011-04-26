
#ifndef UTILS_UTILS_HPP
#define UTILS_UTILS_HPP

#include <sstream>

namespace utils
{

const size_t number_cpus(void);

template <typename T>
std::string to_string (const T& obj)
{
	std::stringstream ss;
	ss << obj;
	return ss.str();
}

}

#endif
