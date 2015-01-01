
#include <vector>
#include <list>

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
   else if(tok.tok == token::Token::THREAD)
      return vm::TYPE_THREAD;
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
   const token start(get()); // type

   // parse modifiers
   bool route(false);
   bool linear(false);
   token nametok;

   while (true) {
      nametok = get();
      if(nametok.tok == token::Token::LINEAR) {
         linear = true;
      } else if(nametok.tok == token::Token::ROUTE) {
         route = true;
      } else if(nametok.tok == token::Token::NAME)
         break;
      else
         throw parser_error(nametok, "unknown type modifier");
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

   if(ast->has_predicate(nametok.str))
      throw parser_error(start, "this predicate is already defined.");
   ast->add_predicate(nametok, linear, std::move(types));
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
   static const token::Token end_tok[1] = {token::Token::DOT};

   get(); // const
   const token name(get());
   if(name.tok != token::Token::NAME)
      throw parser_error(name, "expected a constant name.");
   const token equal(get());
   if(equal.tok != token::Token::EQUAL)
      throw parser_error(equal, "expected an equal = for the constant.");
   expr *e(parse_expr(end_tok, 1));
   ast->constants[name.str] = new constant_definition_expr(name, e);
   const token dot(get());
   if(dot.tok != token::Token::DOT)
      throw parser_error(dot, "constant must be finished with dot.");
}

void
parser::parse_consts()
{
   while(peek().tok == token::Token::CONST)
      parse_const();
}

exists_construct*
parser::parse_exists(const token& start)
{
   vector<var_expr*> vars;
   parse_variables(token::Token::DOT, vars);
   const token lp(get());
   if(lp.tok != token::Token::PAREN_LEFT)
      throw parser_error(lp, "expecting a left parenthesis in exist.");
   vector<fact*> facts;
   parse_subhead(token::Token::PAREN_RIGHT, facts);
   return new exists_construct(start, move(vars), move(facts));
}

void
parser::parse_variables(const token::Token stop, vector<var_expr*>& vars)
{
   while(true) {
      token tok(get());
      // must be a variable
      if(tok.tok != token::Token::VAR)
         throw parser_error(tok, "expecting a comprehension variable.");

      vars.push_back(new var_expr(tok));

      tok = get();
      if(tok.tok == stop)
         break;
      if(tok.tok != token::Token::COMMA)
         throw parser_error(tok, "expecting a comma between comprehension variables.");
   }
}

void
parser::parse_subhead(const token::Token end, vector<fact*>& head_facts)
{
   while(true) {
      const token tok(get());
      switch(tok.tok) {
         case token::Token::VAR:
            throw parser_error(tok, "heads of comprehensions cannot have conditions.");
         case token::token::BANG:
            {
               const token name(get());
               if(name.tok != token::Token::NAME)
                  throw parser_error(name, "expecting a predicate name.");
               head_facts.push_back(parse_head_fact(name, true));
            }
            break;
         case token::Token::NAME:
            head_facts.push_back(parse_head_fact(tok, false));
            break;
         case token::Token::NUMBER:
            throw parser_error(tok, "heads of rules cannot have conditions.");
         default:
            throw parser_error(tok, "unrecognized token in head.");
      }

      const token com(get());
      if(com.tok == end)
         return;
      if(com.tok != token::Token::COMMA)
         throw parser_error(com, "expecting a comma inside the head of the comprehension.");
   }
}

comprehension_construct*
parser::parse_comprehension(const token& start)
{
   token tok(peek());
   vector<var_expr*> compr_vars;

   if(peek().tok != token::Token::BAR) {
      parse_variables(token::Token::BAR, compr_vars);
   } else
      get(); // get |

   // parse comprehension body!
   vector<expr*> conditions;
   vector<fact*> body_facts;

   while(true) {
      parse_body(conditions, body_facts, token::Token::BAR);

      const token com(get());
      if(com.tok == token::Token::BAR)
         break;
      if(com.tok != token::Token::COMMA)
         throw parser_error(com, "here expecting a comma.");
   }

   vector<fact*> head_facts;

   tok = peek();

   if(tok.tok == token::Token::NUMBER && tok.str == "1") {
      get();
      tok = get();
      if(tok.tok != token::Token::CURLY_RIGHT)
         throw parser_error(tok, "expected closing } after 1.");
      return new comprehension_construct(start, move(compr_vars), move(conditions), move(body_facts));
   } else {
      // parse head facts
      vector<fact*> head_facts;
      parse_subhead(token::Token::CURLY_RIGHT, head_facts);
      return new comprehension_construct(start, move(compr_vars),
            move(conditions), move(body_facts), move(head_facts));
   }
}

aggregate_spec*
parser::parse_aggregate_spec()
{
   const token mod(get());
   if(mod.tok != token::Token::NAME && mod.tok != token::Token::MIN)
      throw parser_error(mod, "expecting an aggregate modifier.");
   const token arrow(get());
   if(arrow.tok != token::Token::ARROW)
      throw parser_error(arrow, "expecting an arrow =>.");
   const token var(get());
   if(var.tok != token::Token::VAR)
      throw parser_error(var, "expecting a variable name.");

   return new aggregate_spec(mod, new var_expr(var));
}

aggregate_construct*
parser::parse_aggregate(const token& start)
{
   vector<aggregate_spec*> specs;

   while(true) {
      specs.push_back(parse_aggregate_spec());
      const token tok(get());
      if(tok.tok == token::Token::BAR)
         break;
      if(tok.tok != token::Token::COMMA)
         throw parser_error(tok, "expecting a comma in the aggregate.");
   }

   // parse variables
   token tok(peek());
   vector<var_expr*> agg_vars;

   if(tok.tok == token::Token::BAR)
      get(); // no vars, get |
   else if(tok.tok == token::Token::VAR)
      parse_variables(token::Token::BAR, agg_vars);

   // parse aggregate body!
   vector<expr*> conditions;
   vector<fact*> body_facts;

   while(true) {
      parse_body(conditions, body_facts, token::Token::BAR);

      const token com(get());
      if(com.tok == token::Token::BAR)
         break;
      if(com.tok != token::Token::COMMA)
         throw parser_error(com, "expecting a comma.");
   }

   // parse head facts
   vector<fact*> head_facts;
   parse_subhead(token::Token::BRACKET_RIGHT, head_facts);

   return new aggregate_construct(start, move(specs), move(agg_vars),
            move(conditions), move(body_facts), move(head_facts));
}

fact*
parser::parse_head_fact(const token& name, const bool persistent)
{
   auto it(ast->predicates.find(name.str));
   if(it == ast->predicates.end())
      throw parser_error(name, string("predicate ") + name.str + " was not defined.");
   if(persistent != it->second->is_persistent()) {
      if(persistent)
         throw parser_error(name, "predicate must be persistent.");
      else
         throw parser_error(name, "predicate is linear.");
   }
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
            head_facts.push_back(parse_head_fact(tok, false));
            break;
         case token::Token::NUMBER:
            if(tok.str == "1") {
               const token dot(get());
               if(dot.tok != token::Token::DOT)
                  throw parser_error(tok, "expecting a dot after 1.");
               goto end_head;
            }
            throw parser_error(tok, "Heads of rules cannot have conditions.");
         case token::Token::LOLLI:
            throw parser_error(tok, "Only one implication is allowed in rules.");
         case token::Token::EXISTS:
            constrs.push_back(parse_exists(tok));
            break;
         case token::Token::BRACKET_LEFT:
            constrs.push_back(parse_aggregate(tok));
            break;
         case token::Token::CURLY_LEFT:
            constrs.push_back(parse_comprehension(tok));
            break;
         default:
            throw parser_error(tok, "unrecognized token.");
      }

      const token com(get());
      if(com.tok == token::Token::DOT)
         break;
      if(com.tok != token::Token::COMMA)
         throw parser_error(com, "Expecting a comma.");
   }
end_head:
   if(body_facts.empty()) // axiom
      ast->axioms.push_back(new conditional_axiom(lolli, move(conditions), move(head_facts)));
   else
      ast->rules.push_back(new rule(move(lolli), move(conditions), move(body_facts),
               move(head_facts), move(constrs)));
}

expr*
parser::parse_expr_struct(const token& start)
{
   vector<expr*> exprs;
   if(peek().tok == token::Token::PAREN_RIGHT) {
      get();
      return new struct_expr(start, std::move(exprs));
   }

   while(true) {
      exprs.push_back(parse_atom_expr());
      const token tok(get());
      if(tok.tok == token::Token::PAREN_RIGHT)
         break;
      if(tok.tok != token::Token::COMMA)
         throw parser_error(tok, "expecting a comma.");
   }
   return new struct_expr(start, std::move(exprs));
}

expr*
parser::parse_expr_list(const token& start)
{
   if(peek().tok == token::Token::BRACKET_RIGHT) {
      get();
      return new nil_expr(start);
   }

   vector<expr*> heads;

   while(true) {
      heads.push_back(parse_atom_expr());

      const token tok(get());
      if(tok.tok == token::Token::BAR) {
         expr *tail(parse_atom_expr());
         const token end(get());
         if(end.tok != token::Token::BRACKET_RIGHT)
            throw parser_error(end, "expected end of list ].");
         return new list_expr(start, move(heads), tail);
      }
      if(tok.tok == token::Token::BRACKET_RIGHT)
         return new list_expr(start, move(heads));
      if(tok.tok != token::Token::COMMA)
         throw parser_error(tok, "expecting comma or bar in list expression.");
   }
}

if_expr*
parser::parse_expr_if(const token& start)
{
   static const token::Token cmp_ops[1] = {token::Token::THEN};
   expr *cmp(parse_expr(cmp_ops, sizeof(cmp_ops)/sizeof(token::Token)));

   const token then(get());
   if(then.tok != token::Token::THEN)
      throw parser_error(then, "expected then token.");

   static const token::Token else_ops[2] = {token::Token::ELSE, token::Token::OTHERWISE};
   expr *e1(parse_expr(else_ops, sizeof(else_ops)/sizeof(token::Token)));
   const token se(get());
   if(se.tok != token::Token::ELSE && se.tok != token::Token::OTHERWISE)
      throw parser_error(se, "expected either else or otherwise.");

   static const token::Token end_ops[1] = {token::Token::END};
   expr *e2(parse_expr(end_ops, sizeof(end_ops)/sizeof(token::Token)));

   const token end(get());
   if(end.tok != token::Token::END)
      throw parser_error(end, "expected end token.");

   return new if_expr(start, cmp, e1, e2);
}

expr*
parser::parse_atom_expr()
{
   static token::Token end_tokens[1] = {token::Token::PAREN_RIGHT};

   const token tok(get());
   switch(tok.tok) {
      case token::Token::VAR:
         return new var_expr(tok);
      case token::Token::NAME:
         {
            auto it(ast->constants.find(tok.str));
            if(it == ast->constants.end())
               return parse_expr_funcall(tok);
            else
               return new constant_expr(tok, it->second);
         }
         break;
      case token::Token::FLOAT:
         return parse_expr_funcall(tok);
      case token::Token::NUMBER:
         return new number_expr(tok);
      case token::Token::ADDRESS:
         return new address_expr(tok);
      case token::Token::WORLD:
         return new world_expr(tok);
      case token::Token::BRACKET_LEFT:
         return parse_expr_list(tok);
      case token::Token::IF:
         return parse_expr_if(tok);
      case token::Token::ANONYMOUS_VAR:
         return new var_expr(tok);
      case token::Token::LPACO:
         return parse_expr_struct(tok);
      case token::Token::PAREN_LEFT: {
         expr *ret(parse_expr(end_tokens, 1));
         const token rp(get());
         if(rp.tok != token::Token::PAREN_RIGHT)
            throw parser_error(rp, "expecting matching parenthesis.");
         return ret;
      }
      default: throw parser_error(tok, "expression not handled yet.");
   }
}

static void
process_precedence(std::list<expr*>& atoms, std::list<std::pair<Op, token>>& ops,
      const Op *pre_ops, const size_t num_ops)
{
   auto it_atoms(atoms.begin());
   for(auto it_ops(ops.begin()); it_ops != ops.end(); ) {
      const Op op(it_ops->first);
      const token tok(it_ops->second);
      bool found(false);
      for(size_t i(0); i < num_ops; ++i) {
         if(pre_ops[i] == op) {
            found = true;
            break;
         }
      }
      if(found) {
         expr *b(*it_atoms);
         it_atoms = atoms.erase(it_atoms);
         expr *a(*it_atoms);
         expr *c(new binary_expr(tok, a, b, op));
         *it_atoms = c;
         it_ops = ops.erase(it_ops);
      } else {
         it_atoms++;
         it_ops++;
      }
   }
}

expr*
parser::parse_expr(const token::Token *end_tokens, const size_t end)
{
   std::list<expr*> atoms;
   std::list<std::pair<Op, token>> ops;

   while(true) {
      atoms.push_front(parse_atom_expr());

      const token tok(peek());
      bool found_end(false);

      for(size_t i(0); i < end && !found_end; ++i) {
         if(tok.tok == end_tokens[i]) {
            found_end = true;
            break;
         }
      }
      if(found_end)
         break;

      const token tok_op(get());
      const Op op(parse_operation(tok_op));

      if(op == Op::NO_OP)
         throw parser_error(tok, "expected a valid operation.");

      ops.push_front(std::make_pair(op, tok_op));
   }
   if(atoms.size() == 1)
      return atoms.front();
   static const Op firstprecedence[3] = {Op::MULT, Op::DIVIDE, Op::MOD};
   process_precedence(atoms, ops, firstprecedence, sizeof(firstprecedence)/sizeof(Op));
   static const Op secondprecedence[2] = {Op::PLUS, Op::MINUS};
   process_precedence(atoms, ops, secondprecedence, sizeof(secondprecedence)/sizeof(Op));
   static const Op thirdprecedence[6] = {Op::EQUAL, Op::NOT_EQUAL, Op::GREATER, Op::GREATER_EQUAL, Op::LESSER, Op::LESSER_EQUAL};
   process_precedence(atoms, ops, thirdprecedence, sizeof(thirdprecedence)/sizeof(Op));
   static const Op fourthprecedence[1] = {Op::AND};
   process_precedence(atoms, ops, fourthprecedence, sizeof(fourthprecedence)/sizeof(Op));
   static const Op fifthprecedence[1] = {Op::OR};
   process_precedence(atoms, ops, fifthprecedence, sizeof(fifthprecedence)/sizeof(Op));
   if(atoms.size() > 1)
      throw parser_error(ops.front().second, "invalid operations were found!");
   assert(atoms.size() == 1);
   return atoms.front();
}

fact*
parser::parse_body_fact(const token& name, predicate_definition *pred)
{
   const token lp(get());
   if(lp.tok != token::Token::PAREN_LEFT)
      throw parser_error(lp, "expecting a (.");
   static token::Token end_tokens[2] = {token::Token::PAREN_RIGHT,
      token::Token::COMMA};

   vector<expr*> exprs;
   while(true) {
      expr *e(parse_expr(end_tokens, 2));
      exprs.push_back(e);
      if(exprs.size() > pred->num_fields())
         throw parser_error(name, "too many arguments.");
      const token com(get());
      if(com.tok == token::Token::PAREN_RIGHT) {
         if(exprs.size() < pred->num_fields())
            throw parser_error(name, "too few arguments.");
         return new fact(name, pred, move(exprs));
      }
      if(com.tok != token::Token::COMMA)
         throw parser_error(com, "expecting a comma.");
   }
}

expr*
parser::parse_expr_funcall(const token& name)
{
   auto it(ast->functions.find(name.str));
   size_t args(0);
   function *finternal(nullptr);
   vm::external_function *fexternal(nullptr);

   if(name.str == "float")
      args = 1;
   else if(it == ast->functions.end()) {
      fexternal = vm::lookup_external_function_by_name(name.str);
      if(fexternal == nullptr)
         throw parser_error(name, "cannot find such function.");
      args = fexternal->get_num_args();
   } else {
      finternal = it->second;
      args = finternal->num_args();
   }

   const token lp(get());
   if(lp.tok != token::Token::PAREN_LEFT)
      throw parser_error(lp, "expected a left parenthesis in function call.");
   // 0-arg function call
   const token rp(peek());
   if(rp.tok == token::Token::PAREN_RIGHT) {
      get();
      if(args != 0)
         throw parser_error(name, "expected 0 arguments for this function");
      return new funcall_expr(name, vector<expr*>(), fexternal, finternal);
   }

   static token::Token end_tokens[2] = {token::Token::PAREN_RIGHT,
      token::Token::COMMA};
   vector<expr*> exprs;

   while(true) {
      expr *arg(parse_expr(end_tokens, 2));

      exprs.push_back(arg);

      const token com(get());
      if(com.tok == token::Token::COMMA)
         continue;
      if(com.tok == token::Token::PAREN_RIGHT)
         break;
   }

   if(args != exprs.size())
      throw parser_error(name, "expected a different number of arguments.");

   return new funcall_expr(name, move(exprs), fexternal, finternal);
}

Op
parser::parse_operation(const token& tok)
{
   switch(tok.tok) {
      case token::Token::PLUS:
         return Op::PLUS;
      case token::Token::MINUS:
         return Op::MINUS;
      case token::Token::TIMES:
         return Op::MULT;
      case token::Token::DIVIDE:
         return Op::DIVIDE;
      case token::Token::MOD:
         return Op::MOD;
      case token::Token::EQUAL:
         return Op::EQUAL;
      case token::Token::NOT_EQUAL:
         return Op::NOT_EQUAL;
      case token::Token::LESSER:
         return Op::LESSER;
      case token::Token::LESSER_EQUAL:
         return Op::LESSER_EQUAL;
      case token::Token::GREATER:
         return Op::GREATER;
      case token::Token::GREATER_EQUAL:
         return Op::GREATER_EQUAL;
      case token::Token::OR:
         return Op::OR;
      case token::Token::COMMA:
      case token::Token::PAREN_RIGHT:
         return Op::NO_OP;
      default:
         throw parser_error(tok, "unrecognized operation.");
   }
}

void
parser::parse_body(vector<expr*>& conditions, vector<fact*>& body_facts, const token::Token extra_end)
{
   const token::Token ends[2] = {token::Token::COMMA, extra_end};

   token tok(peek());
   switch(tok.tok) {
      case token::Token::VAR:
      case token::Token::PAREN_LEFT:
      case token::Token::NUMBER:
      case token::Token::ADDRESS:
      case token::Token::BRACKET_LEFT:
      case token::Token::LPACO:
         conditions.push_back(parse_expr(ends, 2));
         break;
      case token::Token::BANG:
         get();
         {
            tok = get();
            auto it(ast->predicates.find(tok.str));
            if(it == ast->predicates.end())
               throw parser_error(tok, "expecting a predicate after the !.");
            predicate_definition *pred(it->second);
            if(!pred->is_persistent())
               throw parser_error(tok, string("predicate ") + tok.str + " is not persistent.");
            body_facts.push_back(parse_body_fact(tok, pred));
         }
         break;
      case token::Token::NAME: {
         auto it(ast->predicates.find(tok.str));
         if(it == ast->predicates.end()) {
            auto it2(ast->constants.find(tok.str));
            if(it2 == ast->constants.end())
               conditions.push_back(parse_expr(ends, 2));
            else
               conditions.push_back(parse_expr(ends, 2));
         } else {
            get();
            body_facts.push_back(parse_body_fact(tok, it->second));
         }
         break;
      }
      case token::Token::COMMA:
         throw parser_error(tok, "comma is not expected here.");
      default:
         throw parser_error(tok, "don't know how to handle (in parse_rule).");
   }
}

void
parser::parse_rule(const token& start)
{
   vector<expr*> conditions;
   vector<fact*> body_facts;

   while(true) {
      parse_body(conditions, body_facts, token::Token::LOLLI);

      const token com(get());
      if(com.tok == token::Token::DOT) {
         // axiom
         if(!conditions.empty())
            throw parser_error(start, "axioms cannot have conditions.");
         for(size_t i(0); i < body_facts.size(); ++i)
            ast->axioms.push_back(new basic_axiom(body_facts[i]));
         return;
      }
      if(com.tok == token::Token::LOLLI)
         return parse_rule_head(com, conditions, body_facts);
      if(com.tok != token::Token::COMMA)
         throw parser_error(com, "here expecting a comma.");
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
parser::parse_priority()
{
   while(true) {
      token tok(peek());

      if(tok.tok == token::Token::PRIORITY) {
         get();
         tok = get();
         switch(tok.tok) {
            case token::Token::ORDER: {
                  tok = get();
                  if(tok.tok == token::Token::DESC)
                     ast->prio_type = vm::PRIORITY_DESC;
                  else if(tok.tok == token::Token::ASC)
                     ast->prio_type = vm::PRIORITY_ASC;
                  else
                     throw parser_error(tok, "invalid ordering type.");
               }
               break;
            case token::Token::CLUSTER: {
                  tok = get();
                  if(tok.tok == token::Token::STATIC)
                     ;
                  else
                     throw parser_error(tok, "invalid cluster type.");
               }
               break;
            default:
               throw parser_error(tok, "don't know how to handle priority statement.");
         }
         tok = get();
         if(tok.tok != token::Token::DOT)
            throw parser_error(tok, "expecting a dot.");
      } else
         return;
   }
}

void
parser::parse_function()
{
   get(); // fun

   const token fname(get());
   if(fname.tok != token::Token::NAME)
      throw parser_error(fname, "expected a function name.");
   const token lp(get());
   if(lp.tok != token::Token::PAREN_LEFT)
      throw parser_error(lp, "expected a left parenthesis.");

   vector<var_expr*> vars;
   vector<vm::type*> types;

   while(true) {
      const token ftype(get());
      types.push_back(parse_type(ftype));

      const token var(get());
      if(var.tok != token::Token::VAR)
         throw parser_error(var, "expected variable name.");

      vars.push_back(new var_expr(var));

      const token end(get());

      if(end.tok == token::Token::PAREN_RIGHT)
         break;
      if(end.tok != token::Token::COMMA)
         throw parser_error(end, "expected a comma or right parenthesis.");
   }

   const token semi(get());
   if(semi.tok != token::Token::SEMICOLON)
      throw parser_error(semi, "Expected a semicollon.");

   const token ret(get());
   vm::type *ret_type(parse_type(ret));

   const token eq(get());
   if(eq.tok != token::Token::EQUAL)
      throw parser_error(eq, "expected = sign.");

   static token::Token end_tokens[1] = {token::Token::DOT};
   expr *body(parse_expr(end_tokens, 1));

   const token dot(get());
   if(dot.tok != token::Token::DOT)
      throw parser_error(dot, "expected a dot.");

   function *f(new function(fname, move(vars), move(types),
            ret_type, body));
   ast->functions[fname.str] = f;
}

void
parser::parse_functions()
{
   while(peek().tok == token::Token::FUN)
      parse_function();
}

void
parser::parse_include()
{
   const token include(get());
   const token filetok(get());
   if(filetok.tok != token::Token::FILENAME)
      throw parser_error(include, "not a valid include declaration.");
   string include_file(filetok.str);
   const size_t last_pos(filename.rfind('/'));
   string dirname = ".";
   if(last_pos != string::npos)
      dirname = string(filename, 0, last_pos);
   include_file = dirname + "/" + include_file;
   std::ifstream f(include_file.c_str());
   if(f.good()) {
      f.close();
      include_files.push_back(include_file);
   } else {
      f.close();
      throw parser_error(filetok, string("filename ") + include_file + " not found.");
   }
}

void
parser::parse_includes()
{
   while(peek().tok == token::Token::INCLUDE)
      parse_include();
}

void
parser::read_includes()
{
   for(auto it(include_files.begin()); it != include_files.end(); ++it) {
      const string& file(*it);
      parser(file, ast);
   }
}

void
parser::parse_program()
{
   parse_includes();

   parse_declarations();

   read_includes();

   parse_priority();

   parse_consts();

   parse_functions();

   parse_rules();
}

parser::parser(const std::string& file, abstract_syntax_tree *tree):
   ast(tree),
   filename(file),
   lex(file)
{
   if(!ast)
      ast = new abstract_syntax_tree();
   vm::init_types();
   vm::init_external_functions();
   parse_program();
   token tok(get());
   if(tok.tok != token::Token::ENDFILE)
      throw parser_error(tok, "expected end of file.");
}

}
