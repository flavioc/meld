
#ifndef COMPILER_LEXER_HPP
#define COMPILER_LEXER_HPP

#include <string>
#include <fstream>
#include <deque>
#include <exception>

#include "mem/base.hpp"
#include "utils/utils.hpp"

namespace compiler
{

struct character {
   char c;
   size_t line;
   size_t col;
};

class token
{
   public:

      typedef enum {
         CURLY_LEFT,
         CURLY_RIGHT,
         BRACKET_LEFT,
         BRACKET_RIGHT,
         DOT,
         COMMA,
         STRING,
         LOLLI,
         NUMBER,
         MINUS,
         BANG,
         DOLLAR,
         QUESTION_MARK,
         ARROW,
         EQUAL,
         LPACO,
         SEMICOLON,
         PROLOG_LOLLI,
         PAREN_LEFT,
         PAREN_RIGHT,
         BAR,
         OR,
         AND,
         DIVIDE,
         PLUS,
         TIMES,
         MOD,
         LESSER_EQUAL,
         NOT_EQUAL,
         LESSER,
         GREATER,
         GREATER_EQUAL,
         ANONYMOUS_VAR,
         VAR,
         NAME,
         ADDRESS,
         ASC,
         ARG,
         WORLD,
         ATRANDOM,
         CPUS,
         CLUSTER,
         INITIAL,
         DESC,
         ATTYPE,
         ORDER,
         STATIC,
         DELAY,
         FILENAME,
         RANDOM,
         INCLUDE,
         ROUTE,
         ACTION,
         LINEAR,
         EXPORT,
         IMPORT,
         FROM, AS, MIN,
         PRIORITY, TRUE, FALSE, NIL,
         IF, THEN, END, OTHERWISE, ELSE,
         BOOL, INT, FLOAT, NODE,
         LIST, CONST,
         TYPE, EXTERN,
         LET, IN, FUN,
         EXISTS,
         ENDFILE
      } Token;

      Token tok;
      size_t line;
      size_t column;
      std::string str;

      explicit token(const Token _tok,
            const size_t _line,
            const size_t _column,
            const std::string& _str):
         tok(_tok), line(_line),
         column(_column),
         str(_str)
      {
      }

      explicit token(const Token _tok, const character x,
            const std::string& _str):
         tok(_tok), line(x.line),
         column(x.col),
         str(_str)
      {
      }
};

class lexer: public mem::base
{
   private:

      std::ifstream stream;
      size_t cur_line;
      size_t cur_col;
      std::deque<character> buffer;

      character peek(void) {
         if(buffer.empty()) {
            if(stream.eof())
               return character{0, cur_line, cur_col};
            char c = stream.peek();
            if(c == '\n')
               return character{c, cur_line + 1, 0};
            else
               return character{c, cur_line, cur_col + 1};
         } else
            return buffer.front();
      }

      void rollback(character c) {
         buffer.push_back(c);
      }

      character get(void) {
         if(buffer.empty()) {
            if(stream.eof())
               return character{0, cur_line, cur_col};
            char c = stream.get();
            if(c == '\n') {
               cur_line++;
               cur_col = 0;
            } else
               cur_col++;
            return character{c, cur_line, cur_col};
         } else {
            character ret(buffer.front());
            buffer.pop_front();
            return ret;
         }
      }

      inline bool parse_word(const std::string& s) {
         for(size_t i(0); i < s.size(); ++i) {
            if(get().c != s[i])
               return false;
         }
         return true;
      }

      std::string lex_string();
      token lex_number_minus(const character);
      token lex_number(const character);
      token lex_equal(const character);
      token lex_semicolon(const character);
      token lex_bar(const character);
      token lex_and(const character);
      token lex_slash(const character);
      token lex_lesser(const character);
      token lex_variable(const character);
      token lex_const(const character);
      token lex_at(const character);
      token lex_arg_asc(const character);
      token lex_cpus_cluster(const character);
      token lex_delay(const character);
      token lex_filename(const character);

      inline bool isend(const character x) const {
         return x.c == 0 || isspace(x.c);
      }

   public:

      token next_token(void);

      explicit lexer(const std::string&);
};

class lexer_error : public std::exception {
   private:

      const std::string msg;
      const size_t line;
      const size_t col;

   public:

      virtual const char *what() const throw()
      {
         return (msg + " (line: " + utils::to_string(line) + " col: " + utils::to_string(col) + ")").c_str();
      }

      explicit lexer_error(const character x, const std::string& _msg) :
         msg(_msg), line(x.line), col(x.col)
      {}
};

}

#endif

