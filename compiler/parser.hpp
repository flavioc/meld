
#ifndef COMPILER_PARSER_HPP
#define COMPILER_PARSER_HPP

#include <exception>
#include <string>
#include <deque>
#include <unordered_map>

#include "compiler/lexer.hpp"
#include "mem/base.hpp"
#include "vm/types.hpp"
#include "vm/predicate.hpp"
#include "vm/program.hpp"

namespace compiler
{

class parser_error : public std::exception {
   private:

      const std::string msg;
      const token tok;

   public:

      virtual const char *what() const noexcept
      {
         return (msg + " (line: " + utils::to_string(tok.line) + " col: " +
               utils::to_string(tok.col) + " " + tok.str + ")").c_str();
      }

      explicit parser_error(const token& x, const std::string& _msg) :
         msg(_msg), tok(x)
      {}
};

class expr: public mem::base
{
   protected:

      const token tok;

   public:

      virtual std::string str() const = 0;

      inline const token get_token() const { return tok; }

      explicit expr(const token& _tok): tok(_tok) {}
};

typedef enum {
   PLUS, MULT, MINUS, DIVIDE, MOD,
   EQUAL, NOT_EQUAL, AND, OR,
   GREATER, GREATER_EQUAL,
   LESSER, LESSER_EQUAL,
   NO_OP
} Op;

static inline std::string op2str(const Op op)
{
   switch(op) {
      case Op::PLUS: return "+";
      case Op::MULT: return "*";
      case Op::MINUS: return "-";
      case Op::DIVIDE: return "/";
      case Op::MOD: return "%";
      case Op::EQUAL: return "=";
      case Op::NOT_EQUAL: return "<>";
      case Op::NO_OP: return "";
      case Op::AND: return "&&";
      case Op::OR: return "||";
      case Op::GREATER: return ">";
      case Op::GREATER_EQUAL: return ">=";
      case Op::LESSER: return "<";
      case Op::LESSER_EQUAL: return "<=";
   }
}

class binary_expr: public expr
{
   private:

      expr *e1;
      expr *e2;
      Op op;

   public:

      virtual std::string str() const {
         return e1->str() + " " + op2str(op) + " " + e2->str();
      }

      explicit binary_expr(const token& tok, expr *_e1, expr *_e2, const Op _op):
         expr(tok), e1(_e1), e2(_e2), op(_op)
      {
         assert(e2);
      }
};

class var_expr: public expr
{
   public:

      virtual std::string str() const {
         return tok.str;
      }

      std::string get_name() const { return tok.str; }

      explicit var_expr(const token& tok):
         expr(tok)
      {}
};

class nil_expr: public expr
{
   public:

      virtual std::string str() const {
         return "[]";
      }

      explicit nil_expr(const token& tok):
         expr(tok)
      {}
};

class list_expr: public expr
{
   private:

      std::vector<expr*> head;
      expr *tail;

   public:

      virtual std::string str() const {
         std::string str = "[";

         for(size_t i(0); i < head.size(); ++i) {
            str += head[i]->str();
            if(i < head.size()-1)
               str += ", ";
         }

         if(tail)
            str += " | " + tail->str();

         return str + "]";
      }

      explicit list_expr(const token& tok,
            std::vector<expr*>&& _head,
            expr *_tail = nullptr):
         expr(tok),
         head(_head),
         tail(_tail)
      {
      }
};

class number_expr: public expr
{
   public:

      virtual std::string str() const {
         return tok.str;
      }

      explicit number_expr(const token& tok):
         expr(tok)
      {
      }
};

class world_expr: public expr
{
   public:

      virtual std::string str() const { return "@world"; }

      explicit world_expr(const token& tok):
         expr(tok)
      {}
};

class address_expr: public expr
{
   public:

      virtual std::string str() const {
         return std::string("@") + tok.str;
      }

      explicit address_expr(const token& tok):
         expr(tok)
      {
      }
};

class if_expr: public expr
{
   private:

      expr *cmp;
      expr *e1;
      expr *e2;

   public:

      virtual std::string str() const
      {
         return std::string("if ") + cmp->str() + " then " + e1->str() + " else " + e2->str() + " end";
      }

      explicit if_expr(const token& tok,
            expr *_cmp, expr *_e1, expr *_e2):
         expr(tok), cmp(_cmp), e1(_e1), e2(_e2)
      {
      }
};

class funcall_expr: public expr
{
   private:

      std::vector<expr*> exprs;

   public:

      virtual std::string str() const {
         std::string ret = tok.str + "(";

         for(size_t i(0); i < exprs.size(); ++i) {
            ret += exprs[i]->str();
            if(i != exprs.size() - 1)
               ret += ", ";
         }

         return ret + ")";
      }

      explicit funcall_expr(const token& tok, std::vector<expr*>&& _exprs):
         expr(tok), exprs(_exprs)
      {
      }
};

class fact: public expr
{
   private:

      const vm::predicate *pred;
      std::vector<expr*> exprs;

   public:

      virtual std::string str() const
      {
         std::string str = "";
         if(pred->is_persistent_pred())
            str += "!";
         str += tok.str + "(";

         for(size_t i(0); i < exprs.size(); ++i) {
            str += exprs[i]->str();
            if(i < exprs.size()-1)
               str += ", ";
         }

         return str + ")";
      }

      explicit fact(const token& tok,
            const vm::predicate *_pred,
            std::vector<expr*>&& _exprs):
         expr(tok), pred(_pred), exprs(_exprs)
      {
      }
};

class constant_expr: public expr
{
   public:

      virtual std::string str() const
      {
         return std::string("const(") + tok.str + ")";
      }

      explicit constant_expr(const token& tok):
         expr(tok)
      {}
};
class constant_definition_expr: public expr
{
   private:

      expr *e;

   public:

      virtual std::string str() const
      {
         return std::string("const ") + tok.str + " = " + e->str() + ".";
      }

      explicit constant_definition_expr(const token& name,
            expr *_e):
         expr(name), e(_e)
      {}
};

class construct: public expr
{
   public:

      explicit construct(const token& tok):
         expr(tok)
      {}
};

class exists_construct: public construct
{
   private:

      std::vector<var_expr*> vars;
      std::vector<fact*> facts;

   public:

      virtual std::string str() const
      {
         std::string str = "exists ";
         for(size_t i(0); i < vars.size(); ++i) {
            str += vars[i]->str();
            if(i < vars.size()-1)
               str += ", ";
         }

         str += ". (";
         for(size_t i(0); i < facts.size(); ++i) {
            str += facts[i]->str();
            if(i < facts.size()-1)
               str += ", ";
         }

         return str + ")";
      }

      explicit exists_construct(const token& tok,
            std::vector<var_expr*>&& _vars,
            std::vector<fact*>&& _facts):
         construct(tok),
         vars(_vars),
         facts(_facts)
      {
      }
};

class aggregate_spec: public expr
{
   private:

      var_expr *var;

   public:

      virtual std::string str() const
      {
         return tok.str + " => " + var->str();
      }

      explicit aggregate_spec(const token& mod,
            var_expr *_var):
         expr(mod), var(_var)
      {
      }
};

class aggregate_construct: public construct
{
   private:

      std::vector<aggregate_spec*> specs;
      std::vector<var_expr*> vars;
      std::vector<expr*> conditions;
      std::vector<fact*> body_facts;
      std::vector<fact*> head_facts;

   public:

      virtual std::string str() const
      {
         std::string str("[");
         for(size_t i(0); i < specs.size(); i++) {
            str += specs[i]->str();
            if(i < specs.size()-1)
               str += ", ";
         }

         str += " | ";

         for(size_t i(0); i < vars.size(); i++) {
            str += vars[i]->str();
            if(i < vars.size()-1)
               str += ", ";
         }

         str += " | ";

         for(size_t i(0); i < body_facts.size(); ++i) {
            str += body_facts[i]->str();
            if(i < body_facts.size()-1)
               str += ", ";
         }
         for(size_t i(0); i < conditions.size(); ++i) {
            str += ", " + conditions[i]->str();
         }
         str += " | ";
         for(size_t i(0); i < head_facts.size(); ++i) {
            str += head_facts[i]->str();
            if(i < head_facts.size()-1)
               str += ", ";
         }

         return str + "}";
      }
      explicit aggregate_construct(const token& tok,
            std::vector<aggregate_spec*>&& _specs,
            std::vector<var_expr*>&& _vars,
            std::vector<expr*>&& _conditions,
            std::vector<fact*>&& _body,
            std::vector<fact*>&& _head):
         construct(tok),
         specs(_specs),
         vars(_vars),
         conditions(_conditions),
         body_facts(_body),
         head_facts(_head)
      {}
};

class comprehension_construct: public construct
{
   private:

      std::vector<var_expr*> vars;
      std::vector<expr*> conditions;
      std::vector<fact*> body_facts;
      std::vector<fact*> head_facts;

   public:

      virtual std::string str() const
      {
         std::string str("{");
         for(size_t i(0); i < vars.size(); i++) {
            str += vars[i]->str();
            if(i < vars.size()-1)
               str += ", ";
         }

         str += " | ";

         for(size_t i(0); i < body_facts.size(); ++i) {
            str += body_facts[i]->str();
            if(i < body_facts.size()-1)
               str += ", ";
         }
         for(size_t i(0); i < conditions.size(); ++i) {
            str += ", " + conditions[i]->str();
         }
         str += " | ";
         for(size_t i(0); i < head_facts.size(); ++i) {
            str += head_facts[i]->str();
            if(i < head_facts.size()-1)
               str += ", ";
         }

         return str + "}";
      }

      explicit comprehension_construct(const token& tok,
            std::vector<var_expr*>&& _vars,
            std::vector<expr*>&& _conditions,
            std::vector<fact*>&& _body,
            std::vector<fact*>&& _head):
         construct(tok),
         vars(_vars),
         conditions(_conditions),
         body_facts(_body),
         head_facts(_head)
      {}

      explicit comprehension_construct(const token& tok,
            std::vector<var_expr*>&& _vars,
            std::vector<expr*>&& _conditions,
            std::vector<fact*>&& _body):
         construct(tok),
         vars(_vars),
         conditions(_conditions),
         body_facts(_body)
      {}
};


class rule: public expr
{
   private:

      std::vector<expr*> body_conditions;
      std::vector<fact*> body_facts;
      std::vector<fact*> head_facts;
      std::vector<construct*> head_constructs;

   public:

      virtual std::string str() const {
         std::string ret;
         for(size_t i(0); i < body_conditions.size(); ++i) {
            ret += body_conditions[i]->str();
            if(i < body_conditions.size()-1)
               ret += ", ";
         }
         if(!body_conditions.empty() && !body_facts.empty())
            ret += ", ";
         for(size_t i(0); i < body_facts.size(); ++i) {
            ret += body_facts[i]->str();
            if(i < body_facts.size()-1)
               ret += ", ";
         }
         ret += " -o ";
         for(size_t i(0); i < head_facts.size(); ++i) {
            ret += head_facts[i]->str();
            if(i < head_facts.size()-1)
               ret += ", ";
         }
         if(!head_facts.empty() && !head_constructs.empty())
            ret += ", ";
         for(size_t i(0); i < head_constructs.size(); ++i) {
            ret += head_constructs[i]->str();
            if(i < head_constructs.size()-1)
               ret += ", ";
         }
         return ret + ".";
      }

      explicit rule(const token& lolli,
            std::vector<expr*>&& _conditions,
            std::vector<fact*>&& _body,
            std::vector<fact*>&& _head,
            std::vector<construct*>&& _constructs):
         expr(lolli), body_conditions(_conditions),
         body_facts(_body), head_facts(_head),
         head_constructs(_constructs)
      {}
};

class axiom: public expr
{
   public:

      explicit axiom(const token& tok): expr(tok) {}
};

class basic_axiom: public axiom
{
   private:

      fact *f;

   public:

      virtual std::string str() const {
         return f->str();
      }

      // no conditions.
      explicit basic_axiom(fact *_f): axiom(_f->get_token()), f(_f) {}
};

class conditional_axiom: public axiom
{
   private:

      std::vector<expr*> conditions;
      std::vector<fact*> head_facts;

   public:

      virtual std::string str() const {
         std::string ret;
         for(size_t i(0); i < conditions.size(); ++i) {
            ret += conditions[i]->str();
            if(i < conditions.size()-1)
               ret += ", ";
         }
         ret += " -o ";
         for(size_t i(0); i < head_facts.size(); ++i) {
            ret += head_facts[i]->str();
            if(i < head_facts.size()-1)
               ret += ", ";
         }
         return ret;
      }

      explicit conditional_axiom(const token& lolli,
            std::vector<expr*>&& _conditions,
            std::vector<fact*>&& _facts):
         axiom(lolli), conditions(_conditions),
         head_facts(_facts)
      {}
};

class function: public expr
{
   private:

      std::vector<var_expr*> vars;
      std::vector<vm::type*> types;
      vm::type *ret_type;
      expr *body;

   public:

      virtual std::string str() const
      {
         std::string str = std::string("fun ") + tok.str + "(";

         for(size_t i(0); i < vars.size(); ++i) {
            str += types[i]->string() + " " + vars[i]->str();
            if(i < vars.size()-1)
               str += ", ";
         }
         return str + ") " + ret_type->string() + " = " + body->str() + ".";
      }

      explicit function(const token& funname,
            std::vector<var_expr*>&& _vars,
            std::vector<vm::type*>&& _types,
            vm::type *_ret_type,
            expr *_body):
         expr(funname),
         vars(_vars),
         types(_types),
         ret_type(_ret_type),
         body(_body)
      {
      }
};

class abstract_syntax_tree: public mem::base
{
   private:

      std::unordered_map<std::string, constant_definition_expr*> constants;
      std::unordered_map<std::string, vm::predicate*> predicates;
      std::unordered_map<std::string, function*> functions;
      std::vector<axiom*> axioms;
      std::vector<rule*> rules;
      vm::priority_type prio_type;

   public:

      void print(std::ostream& out) const
      {
         if(!constants.empty()) {
            out << "CONSTS:\n";
            for(auto it(constants.begin()); it != constants.end(); it++)
               out << it->second->str() << "\n";
         }
         if(!functions.empty()) {
            out << "FUNCTIONS:\n";
            for(auto it(functions.begin()); it != functions.end(); it++)
               out << it->second->str() << "\n";
         }
         if(!axioms.empty()) {
            out << "AXIOMS: \n";
            for(size_t i(0); i < axioms.size(); ++i)
               out << axioms[i]->str() << "\n";
         }
         if(!rules.empty()) {
            out << "\nRULES: \n";
            for(size_t i(0); i < rules.size(); ++i) {
               assert(rules[i] != nullptr);
               out << rules[i]->str() << "\n";
            }
         }
      }

      explicit abstract_syntax_tree() {
      }

      friend class parser;
};

class parser: mem::base
{
   private:

      abstract_syntax_tree *ast{nullptr};
      const std::string filename;
      lexer lex;
      std::deque<token> buffer;
      std::vector<std::string> include_files;

      inline void rollback(const token& tok) {
         buffer.push_back(tok);
      }

      inline token peek() {
         if(buffer.empty()) {
            const token tok(lex.next_token());
            rollback(tok);
            return tok;
         } else
            return buffer.front();
      }

      inline token get() {
         if(buffer.empty())
            return lex.next_token();
         else {
            const token tok(buffer.front());
            buffer.pop_front();
            return tok;
         }
      }

      vm::type *parse_type(const token&);

      void parse_program();
      void parse_include();
      void parse_includes();
      void read_includes();
      void parse_declaration();
      void parse_declarations();
      void parse_const();
      void parse_consts();
      void parse_priority();
      fact *parse_body_fact(const token&, const vm::predicate*);
      void parse_variables(const token::Token, std::vector<var_expr*>&);
      void parse_subhead(const token::Token, std::vector<fact*>&);
      aggregate_spec* parse_aggregate_spec();
      aggregate_construct* parse_aggregate(const token&);
      exists_construct* parse_exists(const token&);
      comprehension_construct* parse_comprehension(const token&);
      fact *parse_head_fact(const token&, const bool);
      Op parse_operation(const token&);
      expr *parse_expr_funcall(const token&);
      expr *parse_expr_list(const token&);
      if_expr *parse_expr_if(const token&);
      expr *parse_atom_expr();
      expr *parse_expr(const token::Token *, const size_t);
      void parse_function();
      void parse_functions();
      void parse_rule_head(const token&, std::vector<expr*>&, std::vector<fact*>&);
      void parse_body(std::vector<expr*>&, std::vector<fact*>&, const token::Token);
      void parse_rule(const token&);
      void parse_rules(void);

   public:

      inline void print(std::ostream& out) const
      {
         ast->print(out);
      }

      abstract_syntax_tree *get_ast() const { return ast; }

      explicit parser(const std::string& file, abstract_syntax_tree *tree = nullptr);
};

}

#endif

