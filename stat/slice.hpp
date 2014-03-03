
#ifndef STAT_SLICE_HPP
#define STAT_SLICE_HPP

#include <ostream>

#include "stat/stat.hpp"
#include "utils/csv_line.hpp"

namespace statistics
{

class slice
{
public:
  
   sched_state state;
   size_t work_queue;
   size_t derived_facts;
   size_t consumed_facts;
   size_t rules_run;
   size_t stolen_nodes;
   size_t sent_facts_same_thread;
   size_t sent_facts_other_thread;
   size_t sent_facts_other_thread_now;
   
   void print_state(utils::csv_line&) const;
   void print_derived_facts(utils::csv_line&) const;
   void print_consumed_facts(utils::csv_line&) const;
   void print_rules_run(utils::csv_line&) const;
   void print_stolen_nodes(utils::csv_line&) const;
   void print_sent_facts_same_thread(utils::csv_line&) const;
   void print_sent_facts_other_thread(utils::csv_line&) const;
   void print_sent_facts_other_thread_now(utils::csv_line&) const;
   
   explicit slice(void):
      state(NOW_IDLE),
      work_queue(0),
      derived_facts(0),
      consumed_facts(0),
      rules_run(0),
      stolen_nodes(0),
      sent_facts_same_thread(0),
      sent_facts_other_thread(0),
      sent_facts_other_thread_now(0)
   {
   }
   
   virtual ~slice(void) {}
   
};

}

#endif
