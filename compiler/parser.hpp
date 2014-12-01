
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

namespace compiler
{

class parser_error : public std::exception {
   private:

      const std::string msg;
      const token tok;

   public:

      virtual const char *what() const throw()
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
   EQUAL, NOT_EQUAL, NO_OP
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

      explicit binary_expr(expr *_e1, expr *_e2, const Op _op):
         expr(_e1->get_token()), e1(_e1), e2(_e2), op(_op)
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

      explicit funcall_expr(const token& tok, const std::vector<expr*>& _exprs):
         expr(tok), exprs(_exprs)
      {
      }
};

class construct: public expr
{
   public:
};

class exists_construct: public construct
{
};

class aggregate_construct: public construct
{
};

class comprehension_construct: public construct
{
};

class fact: public expr
{
   private:

      const vm::predicate *pred;
      std::vector<expr*> exprs;

   public:

      virtual std::string str() const
      {
         std::string str(tok.str + "(");

         for(size_t i(0); i < exprs.size(); ++i) {
            str += exprs[i]->str();
            if(i < exprs.size()-1)
               str += ", ";
         }

         return str + ")";
      }

      explicit fact(const token& tok,
            const vm::predicate *_pred,
            std::vector<expr*> _exprs):
         expr(tok), pred(_pred), exprs(_exprs)
      {
      }
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
         for(size_t i(0); i < head_constructs.size(); ++i) {
            ret += head_constructs[i]->str();
            if(i < head_constructs.size()-1)
               ret += ", ";
         }
         return ret + ".";
      }

      explicit rule(const token& lolli,
            std::vector<expr*> _conditions,
            std::vector<fact*> _body,
            std::vector<fact*> _head,
            std::vector<construct*> _constructs):
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
            std::vector<expr*>& _conditions,
            std::vector<fact*>& _facts):
         axiom(lolli), conditions(_conditions),
         head_facts(_facts)
      {}
};

class parser: mem::base
{
   private:

      lexer lex;
      std::deque<token> buffer;
      std::unordered_map<std::string, vm::predicate*> predicates;
      std::vector<axiom*> axioms;
      std::vector<rule*> rules;

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

      void parse_program(void);
      void parse_declaration(void);
      void parse_declarations(void);
      void parse_const(void);
      void parse_consts(void);
      fact *parse_body_fact(const token&, const vm::predicate*);
      aggregate_construct* parse_aggregate();
      exists_construct* parse_exists();
      comprehension_construct* parse_comprehension();
      fact *parse_head_fact(const token&);
      void parse_body_fact_or_funcall(const token&);
      Op parse_operation();
      expr *parse_expr_funcall(const token&);
      expr *parse_expr_var(const token&);
      expr *parse_expr_paren(const token&);
      expr *parse_expr_num(const token&);
      expr *parse_expr_address(const token&);
      expr *parse_expr();
      void parse_rule_head(const token&, std::vector<expr*>&, std::vector<fact*>&);
      void parse_rule(const token&);
      void parse_rules(void);

   public:

      void print(std::ostream& out) const
      {
         out << "AXIOMS: \n";
         for(size_t i(0); i < axioms.size(); ++i) {
            out << axioms[i]->str() << "\n";
         }
         out << "\nRULES: \n";
         for(size_t i(0); i < rules.size(); ++i) {
            assert(rules[i] != nullptr);
            out << rules[i]->str() << "\n";
         }
      }

      explicit parser(const std::string&);
};

}

#endif

