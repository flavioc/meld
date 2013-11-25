
#include "vm/stat.hpp"

using namespace std;
using namespace utils;

#ifdef CORE_STATISTICS

namespace vm
{
	
static void
sort_and_print_num_predicates(ostream& cout, const string& title, size_t *predicates, vm::all *all)
{
	cout << title << ":" << endl;
	vector< pair<size_t, predicate*> > vec_predicates;
	for(size_t i(0); i < all->PROGRAM->num_predicates(); ++i)
		vec_predicates.push_back(make_pair(predicates[i], all->PROGRAM->get_predicate(i)));
	sort(vec_predicates.begin(), vec_predicates.end(), std::greater<pair<size_t, predicate*> >());
	for(size_t i(0); i < vec_predicates.size(); ++i) {
		if(vec_predicates[i].first == 0)
			break;
		cout << "\t" << vec_predicates[i].second->get_name() << " " << vec_predicates[i].first << endl;
	}
}

static execution_time
sort_and_print_time_predicates(ostream& cout, const string& title, const vector<execution_time>& predicates, vm::all *all)
{
	execution_time total;
	cout << "==> " << title << ":" << endl;
	vector< pair<execution_time, predicate*> > vec_predicates;
	for(size_t i(0); i < all->PROGRAM->num_predicates(); ++i)
		vec_predicates.push_back(make_pair(predicates[i], all->PROGRAM->get_predicate(i)));
	sort(vec_predicates.begin(), vec_predicates.end(), std::greater<pair<execution_time, predicate*> >());
	for(size_t i(0); i < vec_predicates.size(); ++i) {
		if(vec_predicates[i].first.is_zero())
			break;
		total += vec_predicates[i].first;
		cout << "\ttime for " << vec_predicates[i].second->get_name() << " " << vec_predicates[i].first << endl;
	}
	return total;
}

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
   
	sort_and_print_num_predicates(cout, "Proven predicates", stat_predicate_proven, all);
	sort_and_print_num_predicates(cout, "Application predicates", stat_predicate_applications, all);
	sort_and_print_num_predicates(cout, "Successes predicate", stat_predicate_success, all);
   delete []stat_predicate_proven;
   delete []stat_predicate_applications;
   delete []stat_predicate_success;

	cout << "=======Time Statistics===============" << endl;
	const execution_time total_insertion_db = sort_and_print_time_predicates(cout, "Database insertion time per predicate",
		db_insertion_time_predicate, all);
	cout << "=> TOTAL database insertion time: " << total_insertion_db << endl;

	const execution_time total_deletion_db = sort_and_print_time_predicates(cout, "Database deletion time per predicate",
		db_deletion_time_predicate, all);
   cout << "=> TOTAL database deletion time: " << total_deletion_db << endl;
   
	const execution_time total_search_db = sort_and_print_time_predicates(cout, "Database search time per predicate",
		db_search_time_predicate, all);
   cout << "=> TOTAL database search time: " << total_search_db << endl;

	const execution_time total_ts = sort_and_print_time_predicates(cout, "Temporary store search time per predicate",
		ts_search_time_predicate, all);
	cout << "=> TOTAL temporary store search time: " << total_ts << endl;
	
	cout << "=> TOTAL temporary store cleanup time: " << clean_temporary_store_time << endl;
	cout << "=> TOTAL core engine time (calculate rule set): " << core_engine_time << endl;
	
	cout << "========Time Per Rule================" << endl;
   // ordering rules by time spent for each one
	execution_time rule_total;
   vector< pair<execution_time, rule*> > vec_rules_time;
   for(size_t i(0); i < all->PROGRAM->num_rules(); ++i)
      vec_rules_time.push_back(make_pair(rule_times[i], all->PROGRAM->get_rule(i)));

   sort(vec_rules_time.begin(), vec_rules_time.end(), std::greater<pair<execution_time, rule*> >());
   for(size_t i(0); i < all->PROGRAM->num_rules(); ++i) {
		if(vec_rules_time[i].first.is_zero())
			break;
      cout << "Time for rule : " << vec_rules_time[i].first << endl;
      cout << "\t" << vec_rules_time[i].second->get_string() << endl;
		rule_total += vec_rules_time[i].first;
   }
	cout << "=> TOTAL rule time: " << rule_total << endl;
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
	db_insertion_time_predicate.resize(all->PROGRAM->num_predicates());
	db_deletion_time_predicate.resize(all->PROGRAM->num_predicates());
	db_search_time_predicate.resize(all->PROGRAM->num_predicates());
	ts_search_time_predicate.resize(all->PROGRAM->num_predicates());
}

}

#endif
