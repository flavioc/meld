
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
#include "compiler/ast.hpp"

namespace compiler
{

class parser_error : public std::exception {
   private:

      const std::string msg;
      const token tok;

   public:

      virtual const char *what() const noexcept
      {
         return (tok.filename + ": " + msg + " (line: " + utils::to_string(tok.line) + " col: " +
               utils::to_string(tok.col) + " " + tok.str + ")").c_str();
      }

      explicit parser_error(const token& x, const std::string& _msg):
         msg(_msg), tok(x)
      {}
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
      fact *parse_body_fact(const token&, predicate_definition*);
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
      expr *parse_expr_struct(const token&);
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

