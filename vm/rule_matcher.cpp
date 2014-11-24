
#include <algorithm>
#include <iostream>

#include "vm/rule_matcher.hpp"
#include "vm/program.hpp"
#include "vm/state.hpp"

using namespace std;

namespace vm
{

rule_matcher::rule_matcher(void)
{
   predicate_count = mem::allocator<pred_count>().allocate(theProgram->num_predicates());
   memset(predicate_count, 0, theProgram->num_predicates() * sizeof(pred_count));

   rules = mem::allocator<utils::byte>().allocate(theProgram->num_rules());
   memset(rules, 0, sizeof(utils::byte) * theProgram->num_rules());

   bitmap::create(rule_queue, theProgram->num_rules_next_uint());
   rule_queue.clear(theProgram->num_rules_next_uint());

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
   bitmap::destroy(rule_queue, theProgram->num_rules_next_uint());
   assert(rule_queue.empty(theProgram->num_rules_next_uint()));
}

}
