
#include <assert.h>

#include "stat/slice.hpp"
#include "utils/utils.hpp"

using namespace std;
using namespace utils;

namespace statistics
{

void
slice::print_state(csv_line& csv) const
{
   switch(state) {
      case NOW_ACTIVE:
         csv << to_string<size_t>(work_queue + priority_queue);
         break;
      case NOW_IDLE:
         csv << "i";
         break;
      case NOW_SCHED:
         csv << "s";
         break;
      case NOW_ROUND:
         csv << "r";
         break;
      default: assert(false);
   }
}

void
slice::print_work_queue(csv_line& csv) const
{
   csv << to_string<size_t>(work_queue);
}

void
slice::print_processed_facts(csv_line& csv) const
{
   csv << to_string<size_t>(processed_facts);
}

void
slice::print_sent_facts(csv_line& csv) const
{
   csv << to_string<size_t>(sent_facts);
}

void
slice::print_stolen_nodes(csv_line& csv) const
{
   csv << to_string<size_t>(stolen_nodes);
}

void
slice::print_steal_requests(csv_line& csv) const
{
   csv << to_string<size_t>(steal_requests);
}

void
slice::print_priority_queue(csv_line& csv) const
{
   csv << to_string<size_t>(priority_queue);
}

}
