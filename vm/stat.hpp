
#ifndef VM_STAT_HPP
#define VM_STAT_HPP

#include <vector>
#include <iostream>

#include "vm/all.hpp"
#include "utils/time.hpp"
#include "conf.hpp"

#ifdef CORE_STATISTICS
namespace vm
{

class core_statistics {
   public:

      // rule counters
      size_t stat_rules_ok;
      size_t stat_rules_failed;
      bool stat_inside_rule;
      size_t stat_rules_activated;

      // database counters
      size_t stat_db_hits;
      size_t stat_tuples_used;

      // instructions counters
      size_t stat_if_tests;
      size_t stat_if_failed;
      size_t stat_instructions_executed;
      size_t stat_moves_executed;
      size_t stat_ops_executed;

      // predicate counters
      size_t *stat_predicate_proven;
      size_t *stat_predicate_applications;
      size_t *stat_predicate_success;

      // execution time
      utils::execution_time database_insertion_time;
      utils::execution_time database_deletion_time;
      utils::execution_time database_search_time;
      utils::execution_time temporary_store_search_time;
      std::vector<utils::execution_time> rule_times;

   public:

      inline void start_matching(void)
      {
         stat_rules_activated = 0;
         stat_inside_rule = false;
      }

      void print(std::ostream&, vm::all*) const;

      explicit core_statistics(vm::all *);
};

}
#endif

#endif
