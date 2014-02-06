
#include <algorithm>
#include <iostream>

#include "vm/rule_matcher.hpp"
#include "vm/program.hpp"
#include "vm/state.hpp"

using namespace std;

//#define DEBUG_RULES 1

namespace vm
{

void
rule_matcher::register_predicate_availability(const predicate *pred)
{
	for(predicate::rule_iterator it(pred->begin_rules()), end(pred->end_rules()); it != end; it++) {
		vm::rule_id rule_id(*it);
      vm::rule *rule(theProgram->get_rule(rule_id));

      if(rule->as_persistent())
         continue;

      rules[rule_id]++;
		if(rules[rule_id] == rule->num_predicates()) {
#ifdef DEBUG_RULES
			cout << "Rule " << *rule << " activated" << endl;
#endif
         active_bitmap->set_bit(rule_id);
         dropped_bitmap->unset_bit(rule_id);
		}
	}
}

void
rule_matcher::register_predicate_unavailability(const predicate *pred)
{
	for(predicate::rule_iterator it(pred->begin_rules()), end(pred->end_rules()); it != end; it++) {
		vm::rule_id rule_id(*it);
      vm::rule *rule(theProgram->get_rule(rule_id));

      if(rule->as_persistent())
         continue;

		if(rules[rule_id] == rule->num_predicates()) {
#ifdef DEBUG_RULES
			cout << "Rule " << rule << " deactivated" << endl;
#endif
         active_bitmap->unset_bit(rule_id);
         dropped_bitmap->set_bit(rule_id);
		}

		rules[rule_id]--;
	}
}

/* returns true if we did not have any tuples of this predicate */
bool
rule_matcher::register_tuple(tuple *tpl, const derivation_count count, const bool is_new)
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
rule_matcher::deregister_tuple(tuple *tpl, const derivation_count count)
{
   const vm::predicate_id id(tpl->get_predicate_id());
	bool ret(false);
	
#ifdef DEBUG_RULES
	cout << "Remove tuple " << *tpl << " " << count << " " << predicate_count[id] << endl;
#endif
   assert(count > 0);
   assert(predicate_count[id] >= (pred_count)count);

   if(predicate_count[id] == (pred_count)count) {
		ret = true;
		register_predicate_unavailability(tpl->get_predicate());
   }

   predicate_count[id] -= count;
	return ret;
}

rule_matcher::rule_matcher(void)
{
   predicate_count = mem::allocator<pred_count>().allocate(theProgram->num_predicates());
   memset(predicate_count, 0, theProgram->num_predicates() * sizeof(pred_count));

   rules = mem::allocator<utils::byte>().allocate(theProgram->num_rules());
   memset(rules, 0, sizeof(utils::byte) * theProgram->num_rules());

   active_bitmap = create_bitmap(theProgram->num_rules_next_uint());
   dropped_bitmap = create_bitmap(theProgram->num_rules_next_uint());
   active_bitmap->clear(theProgram->num_rules_next_uint());
   dropped_bitmap->clear(theProgram->num_rules_next_uint());

#ifndef NDEBUG
   for(rule_id rid(0); rid < theProgram->num_rules(); ++rid)
   {
      rule *rl(theProgram->get_rule(rid));
      assert(rl->num_predicates() <= 255);
	}
#endif
}

rule_matcher::~rule_matcher(void)
{
   mem::allocator<pred_count>().deallocate(predicate_count, theProgram->num_predicates());
   mem::allocator<utils::byte>().deallocate(rules, theProgram->num_rules());
   delete_bitmap(active_bitmap, theProgram->num_rules_next_uint());
   delete_bitmap(dropped_bitmap, theProgram->num_rules_next_uint());
}

}
