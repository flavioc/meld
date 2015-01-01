

#include <unordered_map>

#include "compiler/ast.hpp"
#include "vm/external.hpp"

namespace compiler
{

using std::string;
using std::unordered_map;
using std::vector;
using vm::type;
using vm::TYPE_NODE;
using vm::TYPE_INT;
using vm::TYPE_FLOAT;
using vm::TYPE_THREAD;
using vm::TYPE_BOOL;

static size_t var_counter(0);

static var_expr*
generate_var()
{
   return new var_expr(std::string("Var") + utils::to_string(var_counter++));
}

localize_error::localize_error(const expr *_e, const std::string& _msg):
   std::runtime_error(_e->get_token().filename + ": " + _msg + " (line: " + utils::to_string(_e->get_token().line) + " col: " +
         utils::to_string(_e->get_token().col) + " " + _e->str() + ")"),
   e(_e), msg(_msg)
{
}

void
basic_axiom::localize()
{
   expr *home(f->get_first());
   if(dynamic_cast<var_expr*>(home))
      return;
   else if(dynamic_cast<address_expr*>(home))
      return;
   else
      throw localize_error(f, "invalid home node.");
}

void
conditional_axiom::localize()
{
   abort();
}

static void
check_home_variable(std::vector<fact*>& body_facts, var_expr *home)
{
   for(fact *f  : body_facts) {
      expr *x(f->get_first());
      var_expr *var(dynamic_cast<var_expr*>(x));
      if(var) {
         if(!var->equal(home))
            throw localize_error(x, std::string("expected variable ") + home->get_name());
      } else
         throw localize_error(x, "expected a variable.");
   }
}

void
comprehension_construct::localize(var_expr *home)
{
   check_home_variable(body_facts, home);
   for(fact *f : body_facts)
      f->localize_arguments(conditions, assignments);
}

void
aggregate_construct::localize(var_expr *home)
{
   check_home_variable(body_facts, home);
   for(fact *f : body_facts)
      f->localize_arguments(conditions, assignments);
}

void
constant_expr::localize_argument(std::vector<expr*>& conditions, std::vector<assignment_expr*>& ass, var_expr *var)
{
   (void)ass;
   conditions.push_back(new binary_expr(token::generated, var, this, Op::EQUAL));
}

void
nil_expr::localize_argument(std::vector<expr*>& conditions, std::vector<assignment_expr*>&, var_expr *var)
{
   conditions.push_back(new binary_expr(token::generated, var, this, Op::EQUAL));
}

void
list_expr::localize_argument(std::vector<expr*>& conditions, std::vector<assignment_expr*>& ass, var_expr *tail_var)
{
   bool process_tail(false);
   for(auto it(head.begin()), end(head.end()); it != end; ++it) {
      expr *h0(*it);
      var_expr *new_tail;
      if(it + 1 == end) {
         var_expr *t(dynamic_cast<var_expr*>(tail));
         if(t)
            new_tail = t;
         else {
            new_tail = generate_var();
            process_tail = true;
         }
      } else
         new_tail = generate_var();
      ass.push_back(new assignment_expr(tail_var->get_token(), new_tail, new tail_expr(tail_var)));
      conditions.push_back(new not_expr(new binary_expr(tail_var->get_token(), tail_var, new nil_expr(tail_var->get_token()), Op::EQUAL)));
      var_expr *h(dynamic_cast<var_expr*>(h0));
      if(h)
         ass.push_back(new assignment_expr(h->get_token(), h, new head_expr(tail_var)));
      else {
         var_expr *head(generate_var());
         ass.push_back(new assignment_expr(tail_var->get_token(), head, new head_expr(tail_var)));
         h0->localize_argument(conditions, ass, head);
      }
      tail_var = new_tail;
   }
   if(process_tail)
      tail->localize_argument(conditions, ass, tail_var);
}

void
binary_expr::localize_argument(std::vector<expr*>& conditions, std::vector<assignment_expr*>&, var_expr* var)
{
   conditions.push_back(new binary_expr(token::generated, var, this, Op::EQUAL));
}

void
number_expr::localize_argument(std::vector<expr*>& conditions, std::vector<assignment_expr*>& ass, var_expr *var)
{
   (void)ass;
   conditions.push_back(new binary_expr(token::generated, var, this, Op::EQUAL));
}

void
fact::localize_arguments(std::vector<expr*>& conditions, std::vector<assignment_expr*>& ass)
{
   auto beg(exprs.begin());
   beg++;
   for(auto it(beg), end(exprs.end()); it != end; ++it) {
      expr *e(*it);
      if(dynamic_cast<var_expr*>(e))
         continue;
      var_expr *new_var(generate_var());
      *it = new_var;
      e->localize_argument(conditions, ass, new_var);
   }
}

void
rule::localize()
{
   expr *home0(body_facts[0]->get_first());
   var_expr *home(dynamic_cast<var_expr*>(home0));
   if(!home)
      throw localize_error(home0, "expected variable as home node.");
   // check use of home variables
   check_home_variable(body_facts, home);
   // check comprehensions and aggregates
   for(construct *c : head_constructs)
      c->localize(home);
   // change non variable arguments to constraints
   var_counter = 0;
   for(fact *f : body_facts)
      f->localize_arguments(body_conditions, body_assignments);
}

void
abstract_syntax_tree::localize()
{
   for(rule *r : rules)
      r->localize();
   for(axiom *a : axioms)
      a->localize();
}

}
