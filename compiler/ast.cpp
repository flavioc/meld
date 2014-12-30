
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


static abstract_syntax_tree *AST{nullptr};
static vector<expr*> untyped_exprs;
static unordered_map<string, vm::type*> var_context;
static bool add_new_vars{true};

type_error::type_error(const expr *_e, const std::string& _msg):
   std::runtime_error(_e->get_token().filename + ": " + _msg + " (line: " + utils::to_string(_e->get_token().line) + " col: " +
         utils::to_string(_e->get_token().col) + " " + _e->str() + ")"),
   e(_e), msg(_msg)
{
}

inline static bool
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

static inline vm::type*
lookup_variable(var_expr *e)
{
   auto it(var_context.find(e->get_name()));
   if(it == var_context.end())
      return nullptr;
   else
      return it->second;
}

static void
process_conditions(std::vector<expr*>& in_constrs, std::vector<assignment_expr*>& out_ass)
{
   // typecheck constraints and assignments:
   // we typecheck constraints with all variables defined
   // if we cannot find typecheck all the constraints
   // we find assignments and define those variables.
   std::vector<expr*> constrs(in_constrs);
   while(!constrs.empty()) {
      // find constraints with variables defined
      for(auto it(constrs.begin()); it != constrs.end(); ) {
         expr *constr(*it);
         if(!has_undefined_variables(constr)) {
            constr->typecheck(TYPE_BOOL);
            it = constrs.erase(it);
         } else {
            std::cout << "Var not ok " << constr->str() << "\n";
            it++;
         }
      }

      // find assignments
      for(auto it(constrs.begin()); it != constrs.end(); ) {
         expr *x(*it);
         binary_expr *bexp(dynamic_cast<binary_expr*>(x));

         if(bexp->looks_like_assignment()) {
            std::cout << "Looks like assignment " << bexp->str() << "\n";
            expr *e1(bexp->get_e1());
            expr *e2(bexp->get_e2());
            var_expr *v1(dynamic_cast<var_expr*>(e1));
            var_expr *v2(dynamic_cast<var_expr*>(e2));
            assignment_expr *ass{nullptr};
            if(v1 && !lookup_variable(v1) && !has_undefined_variables(e2))
               ass = new assignment_expr(x->get_token(), v1, e2);
            else if(v2 && !lookup_variable(v2) && !has_undefined_variables(e1))
               ass = new assignment_expr(x->get_token(), v2, e1);
            if(!ass)
               std::cerr << "Invalid assignment" << std::endl;
            // remove constraint
            it = constrs.erase(it);
            in_constrs.erase(std::find(in_constrs.begin(), in_constrs.end(), x));
            delete x;
            // add assignment
            out_ass.push_back(ass);
            // typecheck assignment
            ass->typecheck();
         } else
            it++;
      }
   }
}

void
comprehension_construct::dotypecheck()
{
   unordered_map<string, vm::type*> copy_context(var_context);

   add_new_vars = true;
   for(auto it(body_facts.begin()), end(body_facts.end()); it != end; ++it)
      (*it)->typecheck();
   add_new_vars = false;
   process_conditions(conditions, assignments);
   add_new_vars = false;
   for(auto it(head_facts.begin()), end(head_facts.end()); it != end; ++it)
      (*it)->typecheck();
   for(auto it(vars.begin()), end(vars.end()); it != end; ++it) {
      var_expr *v(*it);
      const string name(v->get_name());
      auto f(var_context.find(name));
      if(f == var_context.end())
         throw type_error(v, "variable not defined in the comprehension.");
   }
   var_context = copy_context;
}

void
exists_construct::dotypecheck()
{
   unordered_map<string, vm::type*> copy_context(var_context);

   for(auto it(vars.begin()), end(vars.end()); it != end; ++it)
      var_context[(*it)->get_name()] = TYPE_NODE;

   add_new_vars = false;
   for(auto it(facts.begin()), end(facts.end()); it != end; ++it)
      (*it)->typecheck();

   var_context = copy_context;
}

void
aggregate_construct::dotypecheck()
{
   unordered_map<string, vm::type*> copy_context(var_context);

   add_new_vars = true;
   for(auto it(body_facts.begin()), end(body_facts.end()); it != end; ++it)
      (*it)->typecheck();
   add_new_vars = false;
   process_conditions(conditions, assignments);
   for(auto it(vars.begin()), end(vars.end()); it != end; ++it) {
      var_expr *v(*it);
      const string name(v->get_name());
      auto f(var_context.find(name));
      if(f == var_context.end())
         throw type_error(v, "variable not defined in the comprehension.");
   }
   // check aggregate specs
   std::vector<vm::type*> var_types;
   for(auto it(specs.begin()), end(specs.end()); it != end; ++it) {
      aggregate_spec *spec(*it);
      var_expr *var(spec->get_var());
      const string name(var->get_name());
      const AggregateMod mod(spec->get_mod());
      auto f(var_context.find(name));
      if(f == var_context.end()) {
         if(mod == AggregateMod::COUNT)
            var_types.push_back(TYPE_INT);
         else
            throw type_error(spec, "variable not defined.");
      } else {
         switch(mod) {
            case COUNT: var_types.push_back(TYPE_INT); break;
            case COLLECT: var_types.push_back(vm::get_list_type(f->second)); break;
            case MIN:
            case MAX:
            case SUM: {
               vm::type *t(f->second);
               if(t->equal(TYPE_INT) || t->equal(TYPE_FLOAT))
                  var_types.push_back(t);
               else
                  throw type_error(spec, "expected a numeric type.");
            }
            break;
            case NO_MOD: break;
         }
      }
   }

   var_context = copy_context;

   auto typ(var_types.begin());
   for(auto it(specs.begin()); it != specs.end(); ++it, ++typ)
      var_context[(*it)->get_var()->get_name()] = *typ;

   add_new_vars = false;
   for(auto it(head_facts.begin()), end(head_facts.end()); it != end; ++it)
      (*it)->typecheck();

   var_context = copy_context;
}

void
rule::dotypecheck()
{
   std::cout << "Typechecking " << str() << "\n";
   add_new_vars = true;
   for(auto it(body_facts.begin()), end(body_facts.end()); it != end; ++it)
      (*it)->typecheck(nullptr);
   add_new_vars = false;
   process_conditions(body_conditions, body_assignments);
   add_new_vars = false;
   for(auto it(head_facts.begin()), end(head_facts.end()); it != end; ++it)
      (*it)->typecheck();
   for(auto it(head_constructs.begin()), end(head_constructs.end()); it != end; ++it)
      (*it)->typecheck();
   var_context.clear();
}

void
var_expr::dobottom_typecheck()
{
   auto it(var_context.find(get_name()));
   if(it != var_context.end())
      type = it->second;
}

void
var_expr::dotypecheck()
{
   auto it(var_context.find(get_name()));
   if(it == var_context.end()) {
      if(!add_new_vars)
         throw type_error(this, "variable not defined.");
      var_context[get_name()] = type;
   } else {
      if(!type->equal(it->second))
         throw type_error(this, std::string("types not equal ") + type->string() + " -> " + it->second->string());
   }
}

void
funcall_expr::dobottom_typecheck()
{
   if(name() == "float") {
      if(exprs.size() != 1)
         throw type_error(this, "must use only one argument.");
      type = TYPE_FLOAT;
      exprs[0]->typecheck(TYPE_INT);
   } else {
      type = get_ret_type();
      // check argument types
      for(size_t i(0); i < exprs.size(); ++i)
         exprs[i]->typecheck(get_arg_type(i));
   }
}

void
funcall_expr::dotypecheck()
{
   if(name() == "float") {
      if(exprs.size() != 1)
         throw type_error(this, "must use only one argument.");
      if(!type->equal(TYPE_FLOAT))
         throw type_error(this, "must be of type float.");
      exprs[0]->typecheck(TYPE_INT);
   } else {
      // check return type
      vm::type *ret(get_ret_type());
      if(!ret->equal(type))
         throw type_error(this, "invalid return type.");

      // check argument types
      for(size_t i(0); i < exprs.size(); ++i)
         exprs[i]->typecheck(get_arg_type(i));
   }
}

void
constant_expr::dotypecheck()
{
   if(def->get_type() == nullptr)
      def->typecheck(type);
   else {
      if(!def->get_type()->equal(type))
         throw type_error(this, "expecting a different type for constant.");
   }
}

void
constant_expr::dobottom_typecheck()
{
   if(def->get_type())
      type = def->get_type();
}

void
struct_expr::dotypecheck()
{
   if(type->get_type() != vm::FIELD_STRUCT)
      throw type_error(this, "expecting a struct.");
   vm::struct_type *st(dynamic_cast<vm::struct_type*>(type));
   if(st) {
      if(st->get_size() != exprs.size())
         throw type_error(this, "invalid number of struct fields.");
      for(size_t i(0); i < exprs.size(); ++i) {
         vm::type *t(st->get_type(i));
         exprs[i]->typecheck(t);
      }
   } else {
      vm::type *ftype(exprs[0]->bottom_typecheck());
      if(!ftype)
         throw type_error(this, "must be a valid type in struct.");
      for(size_t i(1); i < exprs.size(); ++i)
         exprs[i]->typecheck(ftype);
   }
}

void
list_expr::dotypecheck()
{
   if(type->get_type() != vm::FIELD_LIST)
      throw type_error(this, "expected list type.");
   vm::list_type *lt((vm::list_type*)type);
   
   for(auto it(head.begin()), end(head.end()); it != end; ++it) {
      expr *h(*it);
      h->typecheck(lt->get_subtype());
   }

   if(tail)
      tail->typecheck(type);
}

void
address_expr::dotypecheck()
{
   if(!type->equal(TYPE_NODE))
      throw type_error(this, "expected node type.");
}

void
number_expr::dobottom_typecheck()
{
   if(is_float())
      type = TYPE_FLOAT;
}

void
number_expr::dotypecheck()
{
   if(is_float()) {
      if(!type->equal(TYPE_FLOAT))
         throw type_error(this, "expected float type.");
   } else {
      if(!type->equal(TYPE_FLOAT) && !type->equal(TYPE_INT))
         throw type_error(this, "expected numeric type.");
   }
}

void
assignment_expr::dotypecheck()
{
   add_new_vars = false;
   vm::type *vtype(value->bottom_typecheck());
   if(vtype) {
      type = vtype;
      add_new_vars = true;
      var->typecheck(type);
      add_new_vars = false;
   } else
      throw type_error(this, "cannot find type of assignment value.");
}

void
binary_expr::dobottom_typecheck()
{
   switch(op) {
      case PLUS:
      case MINUS:
      case MULT:
      case DIVIDE:
      case MOD: {
            vm::type *t1(e1->bottom_typecheck());
            vm::type *t2(e2->bottom_typecheck());
            if(t1 && !t2) {
               e2->typecheck(t1);
               type = t1;
            } else if(!t1 && t2) {
               e1->typecheck(t2);
               type = t2;
            } else if(!t1 && !t2)
               return;
            else {
               if(!t1->equal(t2))
                  throw type_error(this, "must use same type.");
               type = t1;
            }
         }
         break;
      case GREATER:
      case LESSER:
      case GREATER_EQUAL:
      case LESSER_EQUAL: {
            vm::type *t1(e1->bottom_typecheck());
            vm::type *t2(e2->bottom_typecheck());
            if(t1 && !t2)
               e2->typecheck(t1);
            else if(!t1 && t2)
               e1->typecheck(t2);
            else if(!t1 && !t2)
               return;
            else if(t1 && t2) {
               if(!t1->equal(t2))
                  throw type_error(this, "must use same type.");
            }
            type = TYPE_BOOL;
            break;
          }
          break;
      case EQUAL:
      case NOT_EQUAL: {
            vm::type *t1(e1->bottom_typecheck());
            vm::type *t2(e2->bottom_typecheck());
            if(t1 && !t2)
               e2->typecheck(t1);
            else if(!t1 && t2)
               e1->typecheck(t2);
            else if(!t1 && !t2) {
               std::cerr << "Missing\n";
               untyped_exprs.push_back(this);
            } else if(!t1->equal(t2))
               throw type_error(this, "must use same type.");
         }
         break;
      case AND:
      case OR:
         e1->typecheck(TYPE_BOOL);
         e2->typecheck(TYPE_BOOL);
         type = TYPE_BOOL;
         break;
      case NO_OP: abort();
   }

}

void
binary_expr::dotypecheck()
{
   switch(op) {
      case PLUS:
      case MINUS:
      case MULT:
      case DIVIDE:
      case MOD:
         if(!type->equal(TYPE_INT) && !type->equal(TYPE_FLOAT))
            throw type_error(this, "must be of numeric type.");
         e1->typecheck(type);
         e2->typecheck(type);
         break;
      case GREATER:
      case LESSER:
      case GREATER_EQUAL:
      case LESSER_EQUAL: {
            if(!type->equal(TYPE_BOOL))
               throw type_error(this, "must be bool type.");
            vm::type *t1(e1->bottom_typecheck());
            vm::type *t2(e2->bottom_typecheck());
            if(t1 && !t2)
               e2->typecheck(t1);
            else if(!t1 && t2)
               e1->typecheck(t2);
            else if(!t1 && !t2) {
               std::cerr << "Missing\n";
               untyped_exprs.push_back(this);
            } else if(!t1->equal(t2))
                  throw type_error(this, "must use same type.");
            if(!t1->equal(TYPE_INT) && !t1->equal(TYPE_FLOAT))
               throw type_error(this, "must be a numeric type.");
            break;
          }
      case EQUAL:
      case NOT_EQUAL: {
            if(!type->equal(TYPE_BOOL))
               throw type_error(this, "must be bool type.");
            vm::type *t1(e1->bottom_typecheck());
            vm::type *t2(e2->bottom_typecheck());
            if(t1 && !t2)
               e2->typecheck(t1);
            else if(!t1 && t2)
               e1->typecheck(t2);
            else if(!t1 && !t2) {
               std::cerr << "Missing\n";
               untyped_exprs.push_back(this);
            } else if(!t1->equal(t2))
                  throw type_error(this, "must use same type.");
         }
         break;
      case AND:
      case OR:
         if(!type->equal(TYPE_BOOL))
            throw type_error(this, "must be of bool type.");
         e1->typecheck(type);
         e2->typecheck(type);
         break;
      case NO_OP: abort();
   }
}

void
fact::dotypecheck()
{
   for(size_t i(0); i < exprs.size(); ++i) {
      expr *x(exprs[i]);
      vm::type *target_type(pred->get_field(i));
      x->typecheck(target_type);
   }
}

void
if_expr::dotypecheck()
{
   cmp->typecheck(vm::TYPE_BOOL);
   e1->typecheck(type);
   e2->typecheck(type);
}

void
if_expr::dobottom_typecheck()
{
   cmp->typecheck(vm::TYPE_BOOL);
   vm::type *t1(e1->bottom_typecheck());
   vm::type *t2(e2->bottom_typecheck());
   if(t1 && !t2) {
      e2->typecheck(t1);
      type = t1;
   } else if(!t1 && t2) {
      e1->typecheck(t2);
      type = t2;
   } else if(!t1 && !t2) {
      untyped_exprs.push_back(this);
   } else if(t1 && t2) {
      if(!t1->equal(t2))
         throw type_error(this, "must be of the same type.");
      type = t1;
   }
}

void
function::dotypecheck()
{
   add_new_vars = true;
   for(size_t i(0); i < vars.size(); ++i)
      vars[i]->typecheck(types[i]);
   add_new_vars = false;
   body->typecheck(ret_type);
   var_context.clear();
}

void
basic_axiom::dotypecheck()
{
   add_new_vars = true;
   f->get_first()->typecheck(TYPE_NODE);
   add_new_vars = false;
   f->typecheck();
}

void
abstract_syntax_tree::typecheck()
{
   untyped_exprs.clear();
   var_context.clear();

   for(auto it(predicates.begin()), end(predicates.end()); it != end; it++) {
      predicate_definition *pred(it->second);

      type* first(pred->get_field(0));
      if(first != TYPE_NODE && first != TYPE_THREAD)
         throw type_error(pred, "first argument meust be either node or thread.");
   }

   for(auto it(functions.begin()), end(functions.end()); it != end; ++it)
      it->second->typecheck();
   for(auto it(rules.begin()), end(rules.end()); it != end; ++it)
      (*it)->typecheck();
   for(auto it(axioms.begin()), end(axioms.end()); it != end; ++it)
      (*it)->typecheck();
   for(auto it(untyped_exprs.begin()), end(untyped_exprs.end()); it != end; ++it) {
      expr *e(*it);
      throw type_error(e, "untyped expression.");
   }
}

abstract_syntax_tree::abstract_syntax_tree()
{
   AST = this;
   vm::init_external_functions();
   // create default predicates.
   initial_predicate1("_init", true, false, vm::TYPE_NODE);
   initial_predicate1("set-priority", true, true, vm::TYPE_NODE);
   initial_predicate4("setcolor", true, true, vm::TYPE_NODE, vm::TYPE_INT, vm::TYPE_INT, vm::TYPE_INT);
   initial_predicate3("setedgelabel", true, true, vm::TYPE_NODE, vm::TYPE_NODE, vm::TYPE_STRING);
   initial_predicate2("write-string", true, true, vm::TYPE_NODE, vm::TYPE_NODE);
   initial_predicate2("add-priority", true, true, vm::TYPE_NODE, vm::TYPE_FLOAT);
   initial_predicate1("schedule-next", true, true, vm::TYPE_NODE);
   initial_predicate2("setColor2", false, true, vm::TYPE_NODE, vm::TYPE_INT);
   initial_predicate1("stop-program", true, true, vm::TYPE_NODE);
   initial_predicate2("set-default-priority", true, true, vm::TYPE_NODE, vm::TYPE_FLOAT);
   initial_predicate1("set-moving", true, true, vm::TYPE_NODE);
   initial_predicate1("set-static", true, true, vm::TYPE_NODE);
   initial_predicate2("set-affinity", true, true, vm::TYPE_NODE, vm::TYPE_NODE);
   initial_predicate2("set-cpu", true, true, vm::TYPE_NODE, vm::TYPE_INT);
   initial_predicate1("remove-priority", true, true, vm::TYPE_NODE);
}

}
