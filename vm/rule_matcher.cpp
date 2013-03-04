
#include <algorithm>
#include <iostream>

#include "vm/rule_matcher.hpp"
#include "vm/program.hpp"
#include "vm/state.hpp"

using namespace std;

//#define DEBUG_RULES 1

namespace vm
{

#ifdef USE_RULE_COUNTING

void
rule_matcher::register_predicate_availability(const predicate *pred)
{
	for(predicate::rule_iterator it(pred->begin_rules()), end(pred->end_rules()); it != end; it++) {
		vm::rule_id rule(*it);

      if(rules[rule].ignore)
         continue;

      rules[rule].total_have++;
		if(rules[rule].total_have == rules[rule].total_needed) {
#ifdef DEBUG_RULES
			cout << "Rule " << rule << " activated" << endl;
#endif
         active_rules.insert(rule);
         dropped_rules.erase(rule);
		}
	}
}

void
rule_matcher::register_predicate_unavailability(const predicate *pred)
{
	for(predicate::rule_iterator it(pred->begin_rules()), end(pred->end_rules()); it != end; it++) {
		vm::rule_id rule(*it);

      if(rules[rule].ignore)
         continue;

		if(rules[rule].total_have == rules[rule].total_needed) {
#ifdef DEBUG_RULES
			cout << "Rule " << rule << " deactivated" << endl;
#endif
         active_rules.erase(rule);
         dropped_rules.insert(rule);
		}

		rules[rule].total_have--;
	}
}

/* returns true if we did not have any tuples of this predicate */
bool
rule_matcher::register_tuple(tuple *tpl, const ref_count count, const bool is_new)
{
   const vm::predicate_id id(tpl->get_predicate_id());
	bool ret(false);
#ifdef DEBUG_RULES
	cout << "Add tuple " << *tpl << endl;
#endif
	if(is_new) {
   	if(predicate_count[id] == 0) {
			ret = true;
      	register_predicate_availability(tpl->get_predicate());
   	}
	}

   predicate_count[id] += count;
	return ret;
}

/* returns true if now we do not have any tuples of this predicate */
bool
rule_matcher::deregister_tuple(tuple *tpl, const ref_count count)
{
   const vm::predicate_id id(tpl->get_predicate_id());
	bool ret(false);
	
#ifdef DEBUG_RULES
	cout << "Remove tuple " << *tpl << endl;
#endif
   assert(predicate_count[id] >= count);

   if(predicate_count[id] == count) {
		ret = true;
		register_predicate_unavailability(tpl->get_predicate());
   }

   predicate_count[id] -= count;
	return ret;
}
#endif

rule_matcher::rule_matcher(vm::program *prog)
{
#ifdef USE_RULE_COUNTING
	predicate_count.resize(prog->num_predicates());
	rules.resize(prog->num_rules());

	fill(predicate_count.begin(), predicate_count.end(), 0);
	
	rule_id rid(0);
	for(rule_vector::iterator it(rules.begin()), end(rules.end());
		it != end;
		it++, rid++)
	{
		rule_matcher_obj& obj(*it);
		
      obj.ignore = prog->get_rule(rid)->as_persistent();
		obj.total_have = 0;
		obj.total_needed = prog->get_rule(rid)->num_predicates();
	}
#endif
}

}
