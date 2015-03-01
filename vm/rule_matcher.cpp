
#include <algorithm>
#include <iostream>

#include "vm/rule_matcher.hpp"
#include "vm/program.hpp"
#include "vm/state.hpp"

using namespace std;

namespace vm {

rule_matcher::rule_matcher(void) {
   rules = mem::allocator<utils::byte>().allocate(theProgram->num_rules());
   memset(rules, 0, sizeof(utils::byte) * theProgram->num_rules());

   bitmap::create(rule_queue, theProgram->num_rules_next_uint());
   rule_queue.clear(theProgram->num_rules_next_uint());

   bitmap::create(predicate_existence, theProgram->num_predicates_next_uint());
   predicate_existence.clear(theProgram->num_predicates_next_uint());
}

rule_matcher::~rule_matcher(void) {
   mem::allocator<utils::byte>().deallocate(rules, theProgram->num_rules());
   bitmap::destroy(rule_queue, theProgram->num_rules_next_uint());
   bitmap::destroy(predicate_existence, theProgram->num_predicates_next_uint());
}
}
