
#include "vm/stat.hpp"

using namespace std;
using namespace utils;

namespace vm
{

void
core_statistics::print(ostream& cout, vm::all *all) const
{
   cout << "Rules:" << endl;
   cout << "\tapplied: " << stat_rules_ok << endl;
   cout << "\tfailed: " << stat_rules_failed << endl;
   cout << "\tfailure rate: " << setprecision (2) << 100 * (float)stat_rules_failed / ((float)(stat_rules_ok + stat_rules_failed)) << "%" << endl;
   cout << "DB hits: " << stat_db_hits << endl;
   cout << "Tuples used: " << stat_tuples_used << endl;
   cout << "If tests: " << stat_if_tests << endl;
   cout << "\tfailed: " << stat_if_failed << endl;
   cout << "Instructions executed: " << stat_instructions_executed << endl;
   cout << "\tmoves: " << stat_moves_executed << endl;
   cout << "\tops: " << stat_ops_executed << endl;
   cout << "Proven predicates:" << endl;
   for(size_t i(0); i < all->PROGRAM->num_predicates(); ++i)
      cout << "\t" << all->PROGRAM->get_predicate(i)->get_name() << " " << stat_predicate_proven[i] << endl;
   cout << "Applications predicate:" << endl;
   for(size_t i(0); i < all->PROGRAM->num_predicates(); ++i)
      cout << "\t" << all->PROGRAM->get_predicate(i)->get_name() << " " << stat_predicate_applications[i] << endl;
   cout << "Successes predicate:" << endl;
   for(size_t i(0); i < all->PROGRAM->num_predicates(); ++i)
      cout << "\t" << all->PROGRAM->get_predicate(i)->get_name() << " " << stat_predicate_success[i] << endl;
   delete []stat_predicate_proven;
   delete []stat_predicate_applications;
   delete []stat_predicate_success;

   cout << "Time Statistics:" << endl;
   cout << "\tdatabase insertion time: " << database_insertion_time << endl;
   cout << "\tdatabase deletion time: " << database_deletion_time << endl;
   cout << "\tdatabase search time: " << database_search_time << endl;
   cout << "\ttemporary store search time: " << temporary_store_search_time << endl;

   // ordering rules by time spent for each one
   vector< pair<execution_time, rule*> > vec_rules_time;
   for(size_t i(0); i < all->PROGRAM->num_rules(); ++i) {
      vec_rules_time.push_back(make_pair(rule_times[i], all->PROGRAM->get_rule(i)));
   }

   sort(vec_rules_time.begin(), vec_rules_time.end(), std::greater<pair<execution_time, rule*> >());
   for(size_t i(0); i < all->PROGRAM->num_rules(); ++i) {
      cout << "\ttime for rule : " << vec_rules_time[i].first << endl;
      cout << "\t\t" << vec_rules_time[i].second->get_string() << endl;
   }
}

core_statistics::core_statistics(vm::all *all)
{
   stat_rules_ok = 0;
   stat_rules_failed = 0;
   stat_db_hits = 0;
   stat_tuples_used = 0;
   stat_if_tests = 0;
   stat_if_failed = 0;
   stat_instructions_executed = 0;
   stat_moves_executed = 0;
   stat_ops_executed = 0;
   stat_predicate_proven = new size_t[all->PROGRAM->num_predicates()];
   stat_predicate_applications = new size_t[all->PROGRAM->num_predicates()];
   stat_predicate_success = new size_t[all->PROGRAM->num_predicates()];
   for(size_t i(0); i < all->PROGRAM->num_predicates(); ++i) {
      stat_predicate_proven[i] = 0;
      stat_predicate_applications[i] = 0;
      stat_predicate_success[i] = 0;
   }
   rule_times.resize(all->PROGRAM->num_rules());
}

}
