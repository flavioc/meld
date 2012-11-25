
#ifndef STAT_STAT_HPP
#define STAT_STAT_HPP

#include <string>

#include "conf.hpp"

namespace statistics
{

enum sched_state
{
   NOW_ACTIVE,
   NOW_IDLE,
   NOW_SCHED,
   NOW_ROUND
};

// frequency in milliseconds to take a slice of thread information   
const unsigned int SLICE_PERIOD = 15;

void set_stat_file(const std::string&);
bool stat_enabled(void);
const std::string get_stat_file(void);

}

#endif

