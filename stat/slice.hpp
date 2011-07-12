
#ifndef STAT_SLICE_HPP
#define STAT_SLICE_HPP

#include <ostream>

#include "sched/thread/state.hpp"
#include "utils/csv_line.hpp"

namespace stat
{

class slice
{
public:
  
   sched::thread_state state;
   size_t work_queue;
   size_t processed_facts;
   size_t sent_facts;
   size_t stealed_nodes;
   size_t steal_requests;
   
   void print_state(utils::csv_line&) const;
   void print_work_queue(utils::csv_line&) const;
   void print_processed_facts(utils::csv_line&) const;
   void print_sent_facts(utils::csv_line&) const;
   void print_stealed_nodes(utils::csv_line&) const;
   void print_steal_requests(utils::csv_line&) const;
   
   explicit slice(void):
      state(sched::THREAD_INACTIVE),
      work_queue(0),
      processed_facts(0),
      sent_facts(0),
      stealed_nodes(0),
      steal_requests(0)
   {
   }
   
   virtual ~slice(void) {}
   
};

}

#endif
