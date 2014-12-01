
#include <vector>

#include "compiler/parser.hpp"

using std::string;
using std::vector;
using std::move;

namespace compiler
{

vm::type*
parser::parse_type(const token& tok)
{
   if(tok.tok == token::Token::NODE)
      return vm::TYPE_NODE;
   else if(tok.tok == token::Token::FLOAT)
      return vm::TYPE_FLOAT;
   else if(tok.tok == token::Token::INT)
      return vm::TYPE_INT;
   else if(tok.tok == token::Token::BOOL)
      return vm::TYPE_BOOL;
   else if(tok.tok == token::Token::STRING)
      return vm::TYPE_STRING;
   else if(tok.tok == token::Token::LIST)
      return new vm::list_type(parse_type(get()));
   else if(tok.tok == token::Token::LPACO) {

      vector<vm::type*> types;
      
      while(true) {
         const token next(get());
         types.push_back(parse_type(next));

         const token com(get());
         if(com.tok == token::Token::PAREN_RIGHT)
            break;
         if(com.tok != token::Token::COMMA)
            throw parser_error(com, "expected a comma ,.");
      }

      vm::struct_type *s(new vm::struct_type(types.size()));
      for(size_t i(0); i < types.size(); ++i)
         s->set_type(i, types[i]);
      return s;
   } else
      throw parser_error(tok, "expected type declaration.");
}

void
parser::parse_declaration()
{
   get(); // type

   // parse modifiers
   bool route(false);
   bool linear(false);
   string name;

   while (true) {
      token tok(get());
      if(tok.tok == token::Token::LINEAR) {
         linear = true;
      } else if(tok.tok == token::Token::ROUTE) {
         route = true;
      } else if(tok.tok == token::Token::NAME) {
         name = tok.str;
         break;
      } else
         throw parser_error(tok, "unknown type modifier");
   }

   // (
   const token lp(get());
   if(lp.tok != token::Token::PAREN_LEFT)
      throw parser_error(lp, "expected a left parenthesis (.");

   vector<vm::type*> types;

   // read types
   while(true) {
      const token tok(get());

      types.push_back(parse_type(tok));

      const token com(get());
      if(com.tok == token::Token::PAREN_RIGHT)
         break;
      if(com.tok != token::Token::COMMA)
         throw parser_error(com, "expected a comma ,.");
   }

   const token dot(get());
   if(dot.tok != token::Token::DOT)
      throw parser_error(dot, "expected a dot ..");

   predicates[name] = vm::predicate::make_predicate_simple(predicates.size(), name, linear, types);
}

void
parser::parse_declarations()
{
   while(peek().tok == token::Token::TYPE)
      parse_declaration();
}

void
parser::parse_const()
{
}

void
parser::parse_consts()
{
   while(peek().tok == token::Token::CONST)
      parse_const();
}

exists_construct*
parser::parse_exists()
{
   return nullptr;
}

comprehension_construct*
parser::parse_comprehension()
{
   return nullptr;
}

aggregate_construct*
parser::parse_aggregate()
{
   return nullptr;
}

fact*
parser::parse_head_fact(const token& name)
{
   auto it(predicates.find(name.str));
   if(it == predicates.end())
      throw parser_error(name, string("predicate ") + name.str + " was not defined.");
   return parse_body_fact(name, it->second);
}

void
parser::parse_rule_head(const token& lolli, vector<expr*>& conditions, vector<fact*>& body_facts)
{
   vector<construct*> constrs;
   vector<fact*> head_facts;

   while(true) {
      const token tok(get());
      switch(tok.tok) {
         case token::Token::VAR:
            throw parser_error(tok, "Heads of rules cannot have conditions.");
         case token::Token::NAME:
            head_facts.push_back(parse_head_fact(tok));
            break;
         case token::Token::NUMBER:
            throw parser_error(tok, "Heads of rules cannot have conditions.");
         case token::Token::LOLLI:
            throw parser_error(tok, "Only one implication is allowed in rules.");
         case token::Token::EXISTS:
            constrs.push_back(parse_exists());
            break;
         case token::Token::BRACKET_LEFT:
            constrs.push_back(parse_aggregate());
            break;
         case token::Token::CURLY_LEFT:
            constrs.push_back(parse_comprehension());
            break;
      }

      const token com(get());
      if(com.tok == token::Token::DOT) {
         if(body_facts.empty()) {
            // axiom
            axioms.push_back(new conditional_axiom(lolli, move(conditions), move(head_facts)));
            return;
         }
         rules.push_back(new rule(move(lolli), move(conditions), move(body_facts),
                  move(head_facts), move(constrs)));
         return;
      }
      if(com.tok != token::Token::COMMA)
         throw parser_error(com, "Expecting a comma.");
   }
}

expr*
parser::parse_expr()
{
   const token tok(get());

   switch(tok.tok) {
      case token::Token::VAR:
         return parse_expr_var(tok);
      case token::Token::NUMBER:
         return parse_expr_num(tok);
      case token::Token::ADDRESS:
         return parse_expr_address(tok);
      case token::Token::NAME:
      case token::Token::FLOAT:
         return parse_expr_funcall(tok);
      default:
         throw parser_error(tok, "expected an expression.");
   }
}

fact*
parser::parse_body_fact(const token& name, const vm::predicate *pred)
{
   const token lp(get());
   if(lp.tok != token::Token::PAREN_LEFT)
      throw parser_error(lp, "expecting a (.");

   vector<expr*> exprs;
   while(true) {
      expr *e(parse_expr());
      exprs.push_back(e);
      const token com(get());
      if(com.tok == token::Token::PAREN_RIGHT)
         return new fact(name, pred, move(exprs));
      if(com.tok != token::Token::COMMA)
         throw parser_error(com, "expecting a comma.");
   }
}

expr*
parser::parse_expr_funcall(const token& name)
{
   const token lp(get());
   if(lp.tok != token::Token::PAREN_LEFT)
      throw parser_error(lp, "expected a left parenthesis in function call.");
   // 0-arg function call
   const token rp(peek());
   if(rp.tok == token::Token::PAREN_RIGHT) {
      get();
      return new funcall_expr(name, vector<expr*>());
   }

   vector<expr*> exprs;

   while(true) {
      expr *arg(parse_expr());

      exprs.push_back(arg);

      const token com(get());
      if(com.tok == token::Token::COMMA)
         continue;
      if(com.tok == token::Token::PAREN_RIGHT)
         break;
   }

   return new funcall_expr(name, move(exprs));
}

void
parser::parse_body_fact_or_funcall(const token& name)
{
   auto it(predicates.find(name.str));
   if(it == predicates.end())
      parse_expr_funcall(name);
   else
      parse_body_fact(name, it->second);
}

Op
parser::parse_operation()
{
   const token tok(peek());
   switch(tok.tok) {
      case token::Token::PLUS:
         get();
         return Op::PLUS;
      case token::Token::MINUS:
         get();
         return Op::MINUS;
      case token::Token::TIMES:
         get();
         return Op::MULT;
      case token::Token::DIVIDE:
         get();
         return Op::DIVIDE;
      case token::Token::MOD:
         get();
         return Op::MOD;
      case token::Token::EQUAL:
         get();
         return Op::EQUAL;
      case token::Token::NOT_EQUAL:
         get();
         return Op::NOT_EQUAL;
      case token::Token::COMMA:
      case token::Token::PAREN_RIGHT:
         return Op::NO_OP;
      default:
         throw parser_error(tok, "unrecognized operation.");
   }
}

expr*
parser::parse_expr_paren(const token& start)
{
   expr *e(parse_expr());

   const token rp(get());
   if(rp.tok != token::Token::PAREN_RIGHT)
      throw parser_error(rp, "parenthesis needs to be enclosed.");

   return e;
}

expr*
parser::parse_expr_var(const token& start)
{
   var_expr *v(new var_expr(start));
   Op op(parse_operation());
   if(op == Op::NO_OP)
      return v;

   expr *e2(parse_expr());
   assert(v);
   assert(e2);
   return new binary_expr(v, e2, op);
}

expr*
parser::parse_expr_address(const token& start)
{
   address_expr *n(new address_expr(start));
   Op op(parse_operation());
   if(op == Op::NO_OP)
      return n;

   expr *e2(parse_expr());
   return new binary_expr(n, e2, op);
}

expr*
parser::parse_expr_num(const token& start)
{
   number_expr *n(new number_expr(start));
   Op op(parse_operation());
   if(op == Op::NO_OP)
      return n;

   expr *e2(parse_expr());
   return new binary_expr(n, e2, op);
}

void
parser::parse_rule(const token& start)
{
   vector<expr*> conditions;
   vector<fact*> body_facts;

   while(true) {
      const token tok(get());

      switch(tok.tok) {
         case token::Token::VAR:
            conditions.push_back(parse_expr_var(tok));
            break;
         case token::Token::PAREN_LEFT:
            conditions.push_back(parse_expr_paren(tok));
            break;
         case token::Token::NUMBER:
            conditions.push_back(parse_expr_num(tok));
            break;
         case token::Token::NAME: {
            auto it(predicates.find(tok.str));
            if(it == predicates.end())
               conditions.push_back(parse_expr_funcall(tok));
            else
               body_facts.push_back(parse_body_fact(tok, it->second));
            break;
         }
         case token::Token::COMMA:
            throw parser_error(tok, "comma is not expected here.");
      }

      const token com(get());
      if(com.tok == token::Token::DOT) {
         // axiom
         if(!conditions.empty())
            throw parser_error(start, "axioms cannot have conditions.");
         for(size_t i(0); i < body_facts.size(); ++i)
            axioms.push_back(new basic_axiom(body_facts[i]));
         return;
      }
      if(com.tok == token::Token::LOLLI)
         return parse_rule_head(com, conditions, body_facts);
      if(com.tok != token::Token::COMMA)
         throw parser_error(com, "expecting a comma.");
   }
}

void
parser::parse_rules()
{
   while(true) {
      const token tok(peek());

      if(tok.tok == token::Token::ENDFILE)
         break;

      parse_rule(tok);
   }
}

void
parser::parse_program(void)
{
   parse_declarations();

   parse_consts();

   parse_rules();
}

parser::parser(const std::string& file):
   lex(file)
{
   vm::init_types();
   parse_program();
   token tok(get());
   if(tok.tok != token::Token::ENDFILE)
      throw parser_error(tok, "expected end of file.");
}

}
