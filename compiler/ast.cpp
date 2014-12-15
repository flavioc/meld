
#include <unordered_map>

#include "compiler/ast.hpp"
#include "vm/external.hpp"

namespace compiler
{

using std::string;
using std::unordered_map;
using vm::type;
using vm::TYPE_NODE;
using vm::TYPE_INT;
using vm::TYPE_FLOAT;
using vm::TYPE_THREAD;
using vm::TYPE_BOOL;


static abstract_syntax_tree *AST{nullptr};
static unordered_map<string, vm::type*> var_context;

type_error::type_error(const expr *_e, const std::string& _msg):
   std::runtime_error(_e->get_token().filename + ": " + _msg + " (line: " + utils::to_string(_e->get_token().line) + " col: " +
         utils::to_string(_e->get_token().col) + " " + _e->str() + ")"),
   e(_e), msg(_msg)
{
}

void
rule::dotypecheck()
{
   for(auto it(body_facts.begin()), end(body_facts.end()); it != end; ++it)
      (*it)->typecheck(nullptr);
   for(auto it(head_facts.begin()), end(head_facts.end()); it != end; ++it)
      (*it)->typecheck(nullptr);
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
   if(it == var_context.end())
      var_context[get_name()] = type;
   else {
      if(!type->equal(it->second))
         throw type_error(this, std::string("types not equal ") + type->string() + " -> " + it->second->string());
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
      if(finternal) {
         vm::type *ret(finternal->get_ret_type());
         if(!ret->equal(type))
            throw type_error(this, "invalid return type.");
      } else if(fexternal) {
         vm::type *ret(fexternal->get_return_type());
         if(!ret->equal(type))
            throw type_error(this, "invalid return type.");
      }

      // check argument types
      for(size_t i(0); i < exprs.size(); ++i) {
         vm::type *e_type(nullptr);
         if(finternal)
            e_type = finternal->get_arg_type(i);
         else if(fexternal)
            e_type = fexternal->get_arg_type(i);
         assert(e_type);
         exprs[i]->typecheck(e_type);
      }
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
struct_expr::dotypecheck()
{
   if(type->get_type() != vm::FIELD_STRUCT)
      throw type_error(this, "expecting a struct.");
   vm::struct_type *st((vm::struct_type*)type);
   if(st->get_size() != exprs.size())
      throw type_error(this, "invalid number of struct fields.");
   for(size_t i(0); i < exprs.size(); ++i) {
      vm::type *t(st->get_type(i));
      exprs[i]->typecheck(t);
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
      case EQUAL:
      case NOT_EQUAL: {
            vm::type *t1(e1->bottom_typecheck());
            vm::type *t2(e2->bottom_typecheck());
            if(t1 && !t2)
               e2->typecheck(t1);
            else if(!t1 && t2)
               e1->typecheck(t2);
            else
               abort();
         }
         break;
      case AND:
      case OR:
         if(!type->equal(TYPE_BOOL))
            throw type_error(this, "must be of bool type.");
         e1->typecheck(type);
         e2->typecheck(type);
         break;
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
function::dotypecheck()
{
   for(size_t i(0); i < vars.size(); ++i)
      vars[i]->typecheck(types[i]);
   body->typecheck(ret_type);
   var_context.clear();
}

void
basic_axiom::dotypecheck()
{
   f->typecheck();
}

void
abstract_syntax_tree::typecheck()
{
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
