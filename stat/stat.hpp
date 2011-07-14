
#ifndef STAT_STAT_HPP
#define STAT_STAT_HPP

#include <string>

namespace stat
{

// frequency in milliseconds to take a slice of thread information   
const unsigned int SLICE_PERIOD = 5;

void set_stat_file(const std::string&);
bool stat_enabled(void);
const std::string get_stat_file(void);

}

#endif

