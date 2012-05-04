
#include <assert.h>

#include "stat/stat.hpp"

using namespace std;

namespace statistics
{

static bool debug_file_set(false);
static string debug_file_name;

void
set_stat_file(const string& file)
{
	debug_file_name = file;
	debug_file_set = true;
}

const string
get_stat_file(void)
{
   assert(stat_enabled());
   return debug_file_name;
}

bool
stat_enabled(void)
{
	return debug_file_set;
}

}
