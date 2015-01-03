
#include <iostream>
#include <unordered_map>
#include <algorithm>

#include "compiler/asm.hpp"

namespace compiler
{

using std::unordered_map;
using std::vector;

static unordered_map<std::string, value*> var_context;

static inline bool
has_undefined_variables(expr *e)
{
   bool found_undef_variables(false);
   e->iterate([&found_undef_variables] (expr *x) -> expr::iterate_ret {
         if(var_expr *var = dynamic_cast<var_expr*>(x)) {
            auto it(var_context.find(var->get_name()));
            if(it == var_context.end())
               found_undef_variables = true;
            return expr::iterate_ret::HALT;
         } else
            return expr::iterate_ret::CONTINUE;
      });
   return found_undef_variables;
}

static vector<expr*>
get_free_conditions(vector<expr*>& conditions)
{
   vector<expr*> free;
   std::remove_copy_if(conditions.begin(), conditions.end(), std::back_inserter(free),
         has_undefined_variables);
   return free;
}

compiled_rule::compiled_rule(rule *r):
   orig_rule(r)
{
   for(fact * f : orig_rule->body_facts) {
      predicate_definition *def(f->get_def());
      auto it(find(body_predicates.begin(), body_predicates.end(), def));
      if(it == body_predicates.end())
         body_predicates.push_back(def);
   }

   // compile rule
   var_context.clear();

   std::vector<expr*> conditions(orig_rule->body_conditions);
   std::vector<assignment_expr*> assignments(orig_rule->body_assignments);
   std::vector<fact*> facts(orig_rule->body_facts);

#if 0
   std::vector<expr*> free_conditions(get_free_conditions(conditions));
   for(expr *e : free_conditions) {
      compile_expression(e, instrs);
   }
#endif
}

compiled_predicate::compiled_predicate(predicate_definition *def,
      std::vector<rule*>& usable_rules)
{
}

void
compiled_program::compile()
{
   for(rule *r : ast->rules) {
      if(!r->is_persistent())
         rules.push_back(new compiled_rule(r));
   }
   for(auto it : ast->predicates) {
      std::vector<rule*> usable_rules;
      predicate_definition *def(it.second);
      std::copy_if(ast->rules.begin(), ast->rules.end(),
            std::back_inserter(usable_rules),
            [def](rule *r) {
               return r->is_persistent() && r->uses_predicate(def);
            });
      processes.push_back(new compiled_predicate(def, usable_rules));
   }
}

}
