
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

      // insertion time per predicate
		std::vector<utils::execution_time> db_insertion_time_predicate;
		
		// deletion time per predicate
		std::vector<utils::execution_time> db_deletion_time_predicate;
		
		// search time for each predicate
		std::vector<utils::execution_time> db_search_time_predicate;
		
		// search time for each predicate
		std::vector<utils::execution_time> ts_search_time_predicate;
		
		// time to clean temporary store
		utils::execution_time clean_temporary_store_time;
		
		utils::execution_time core_engine_time;
		
		// run time per rule
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
