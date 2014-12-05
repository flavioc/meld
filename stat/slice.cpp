
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
         csv << to_string<size_t>(work_queue);
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
slice::print_derived_facts(csv_line& csv) const
{
   csv << to_string<size_t>(derived_facts);
}

void
slice::print_consumed_facts(csv_line& csv) const
{
   csv << to_string<size_t>(consumed_facts);
}

void
slice::print_rules_run(csv_line& csv) const
{
   csv << to_string<size_t>(rules_run);
}

void
slice::print_stolen_nodes(csv_line& csv) const
{
   csv << to_string<size_t>(stolen_nodes);
}

void
slice::print_sent_facts_same_thread(csv_line& csv) const
{
   csv << to_string<size_t>(sent_facts_same_thread);
}

void
slice::print_sent_facts_other_thread(csv_line& csv) const
{
   csv << to_string<size_t>(sent_facts_other_thread);
}

void
slice::print_sent_facts_other_thread_now(csv_line& csv) const
{
   csv << to_string<size_t>(sent_facts_other_thread_now);
}

void
slice::print_priority_nodes_thread(csv_line& csv) const
{
   csv << to_string<size_t>(priority_nodes_thread);
}

void
slice::print_priority_nodes_others(csv_line& csv) const
{
   csv << to_string<size_t>(priority_nodes_others);
}

void
slice::print_thread_transactions(csv_line& csv) const
{
   csv << to_string<size_t>(thread_transactions);
}

void
slice::print_all_transactions(csv_line& csv) const
{
   csv << to_string<size_t>(all_transactions);
}

}
