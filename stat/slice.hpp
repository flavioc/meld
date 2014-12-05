
#ifndef STAT_SLICE_HPP
#define STAT_SLICE_HPP

#include <ostream>

#include "stat/stat.hpp"
#include "utils/csv_line.hpp"
#include "utils/utils.hpp"

namespace statistics
{

struct slice
{
public:
  
   sched_state state = NOW_IDLE;
   size_t work_queue = 0;
   size_t derived_facts = 0;
   size_t consumed_facts = 0;
   size_t rules_run = 0;
   size_t stolen_nodes = 0;
   size_t thread_transactions = 0;
   size_t all_transactions = 0;
   size_t sent_facts_same_thread = 0;
   size_t sent_facts_other_thread = 0;
   size_t sent_facts_other_thread_now = 0;
   size_t priority_nodes_thread = 0;
   size_t priority_nodes_others = 0;
   int64_t bytes_used = 0;
   size_t node_lock_ok = 0;
   size_t node_lock_fail = 0;
   
   void print_state(utils::csv_line&) const;
   void print_derived_facts(utils::csv_line&) const;
   void print_consumed_facts(utils::csv_line&) const;
   void print_thread_transactions(utils::csv_line&) const;
   void print_all_transactions(utils::csv_line&) const;
   void print_rules_run(utils::csv_line&) const;
   void print_stolen_nodes(utils::csv_line&) const;
   void print_sent_facts_same_thread(utils::csv_line&) const;
   void print_sent_facts_other_thread(utils::csv_line&) const;
   void print_sent_facts_other_thread_now(utils::csv_line&) const;
   void print_priority_nodes_thread(utils::csv_line&) const;
   void print_priority_nodes_others(utils::csv_line&) const;
   void print_bytes_used(utils::csv_line& csv) const
   {
      csv << utils::to_string<int64_t>(bytes_used);
   }
   void print_node_lock_ok(utils::csv_line& csv) const
   {
      csv << utils::to_string<size_t>(node_lock_ok);
   }
   void print_node_lock_fail(utils::csv_line& csv) const
   {
      csv << utils::to_string<size_t>(node_lock_fail);
   }
};

}

#endif
