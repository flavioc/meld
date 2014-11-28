
#include "compiler/lexer.hpp"

using std::string;
using std::ifstream;
using std::make_pair;
using std::pair;

namespace compiler
{

string
lexer::lex_string()
{
   char max_string[1024];
   size_t i(0);
   while(true) {
      character x(get());
      if(x.c == '"')
         break;
      max_string[i++] = x.c;
   }
   return string(max_string);
}

token
lexer::lex_number(const character start)
{
   char max_string[1024];
   size_t i(0);

   max_string[i++] = start.c;

   while(isdigit(peek().c))
      max_string[i++] = get().c;

   if(max_string[0] == '-' && i == 1)
      throw lexer_error(start, "Cannot identify next token. Did you mean a-b?");

   if(peek().c == '.') {
      size_t floats(0);

      character dot(get());

      while(isdigit(peek().c)) {
         max_string[i + floats + 1] = get().c;
         floats++;
      }

      if(floats == 0) {
         max_string[i++] = '\0';
         rollback(dot);
         return token(token::Token::NUMBER, start, max_string);
      } else {
         max_string[i++] = '.';
         i += floats;
         max_string[i++] = '\0';
         return token(token::Token::NUMBER, start, max_string);
      }
   }

   max_string[i++] = '\0';
   return token(token::Token::NUMBER, start, max_string);
}

token
lexer::lex_number_minus(const character start)
{
   character next(get());
   if(isend(next))
      return token(token::Token::MINUS, start, "-");

   if(next.c == 'o')
      return token(token::Token::LOLLI, start, "-o");

   return lex_number(start);
}

token
lexer::lex_equal(const character start)
{
   if(peek().c == '>') {
      get();
      return token(token::Token::ARROW, start, "=>");
   }

   return token(token::Token::EQUAL, start, "=");
}

token
lexer::lex_semicolon(const character start)
{
   if(peek().c == '(') {
      get();
      return token(token::Token::LPACO, start, ":(");
   }
   if(peek().c == ':') {
      get();
      return token(token::Token::PROLOG_LOLLI, start, ":-");
   }
   return token(token::Token::SEMICOLON, start, ":");
}

token
lexer::lex_bar(const character start)
{
   if(peek().c == '|') {
      get();
      return token(token::Token::OR, start, "||");
   }
   return token(token::Token::BAR, start, "|");
}

token
lexer::lex_and(const character start)
{
   if(get().c != '&')
      throw lexer_error(start, "Cannot identify next token. Did you mean &&?");
   return token(token::Token::AND, start, "&&");
}

token
lexer::lex_slash(const character start)
{
   switch(peek().c) {
      case '*':
         // skip comment
         get();
         while(true) {
            const character x(get());
            if(x.c == 0)
               return token(token::Token::ENDFILE, start, "");

            if(x.c == '*' && get().c == '/')
               return next_token();
         }
      case '/':
         // find newline
         get();
         while(peek().c != '\n' && peek().c != 0)
            get();
         return next_token();
      default:
         return token(token::Token::DIVIDE, start, "/");
   }
}

token
lexer::lex_lesser(const character start)
{
   switch(peek().c) {
      case '=': return token(token::Token::LESSER_EQUAL, start, "<=");
      case '>': return token(token::Token::NOT_EQUAL, start, "<>");
      default: return token(token::Token::LESSER, start, "<");
   }
}

token
lexer::lex_variable(const character start)
{
   char max_string[1024];
   size_t i(0);
   max_string[i++] = start.c;

   while(true) {
      const character x(peek());

      if(isalpha(x.c) || isdigit(x.c) || x.c == '_') {
         max_string[i++] = x.c;
         get();
         continue;
      } else {
         max_string[i] = '\0';
         return token(token::Token::VAR, start, max_string);
      }
   }
}

token
lexer::lex_const(const character start)
{
   char max_string[1024];
   size_t i(0);
   max_string[i++] = start.c;

   while(true) {
      const character x(peek());

      if(isalpha(x.c) || isdigit(x.c) || x.c == '_' || x.c == '-' || x.c == '?') {
         max_string[i++] = x.c;
         get();
         continue;
      } else {
         max_string[i] = '\0';
         const string name(max_string);

         if(name == "type")
            return token(token::Token::TYPE, start, name);
         else if(name == "exists")
            return token(token::Token::EXISTS, start, name);
         else if(name == "extern")
            return token(token::Token::EXTERN, start, name);
         else if(name == "const")
            return token(token::Token::CONST, start, name);
         else if(name == "bool")
            return token(token::Token::BOOL, start, name);
         else if(name == "int")
            return token(token::Token::INT, start, name);
         else if(name == "float")
            return token(token::Token::FLOAT, start, name);
         else if(name == "node")
            return token(token::Token::NODE, start, name);
         else if(name == "string")
            return token(token::Token::STRING, start, name);
         else if(name == "list")
            return token(token::Token::LIST, start, name);
         else if(name == "include")
            return token(token::Token::INCLUDE, start, name);
         else if(name == "random")
            return token(token::Token::RANDOM, start, name);
         else if(name == "route")
            return token(token::Token::ROUTE, start, name);
         else if(name == "action")
            return token(token::Token::ACTION, start, name);
         else if(name == "linear")
            return token(token::Token::LINEAR, start, name);
         else if(name == "export")
            return token(token::Token::EXPORT, start, name);
         else if(name == "import")
            return token(token::Token::IMPORT, start, name);
         else if(name == "from")
            return token(token::Token::FROM, start, name);
         else if(name == "as")
            return token(token::Token::AS, start, name);
         else if(name == "let")
            return token(token::Token::LET, start, name);
         else if(name == "in")
            return token(token::Token::IN, start, name);
         else if(name == "end")
            return token(token::Token::END, start, name);
         else if(name == "if")
            return token(token::Token::IF, start, name);
         else if(name == "then")
            return token(token::Token::THEN, start, name);
         else if(name == "else")
            return token(token::Token::ELSE, start, name);
         else if(name == "otherwise")
            return token(token::Token::OTHERWISE, start, name);
         else if(name == "fun")
            return token(token::Token::FUN, start, name);
         else if(name == "min")
            return token(token::Token::MIN, start, name);
         else if(name == "priority")
            return token(token::Token::PRIORITY, start, name);
         else if(name == "true")
            return token(token::Token::TRUE, start, name);
         else if(name == "false")
            return token(token::Token::FALSE, start, name);
         else if(name == "nil")
            return token(token::Token::NIL, start, name);
         else
            return token(token::Token::NAME, start, name);
      }
   }
}

token
lexer::lex_arg_asc(const character start)
{
   get(); // consume 'a'
   if(peek().c == 'r') {
      get();
      if(get().c != 'g')
         throw lexer_error(start, "Parser error: should be @argX.");
      char max_string[32];
      size_t i(0);
      // get id of arg
      while(true) {
         if(isdigit(peek().c) && i < 32)
            max_string[i++] = get().c;
         else if(i == 0)
            throw lexer_error(start, "Parser error: should be @argX.");
         else {
            max_string[i] = '\0';
            return token(token::Token::ARG, start, max_string);
         }
      }
   } else if(peek().c == 's') {
      get();
      if(get().c != 'c')
         throw lexer_error(start, "Parser error: should be @asc.");
      return token(token::Token::ASC, start, "@asc");
   } else
      throw lexer_error(start, "Parser error: should be either @argX or @asc.");
}

token
lexer::lex_cpus_cluster(const character start)
{
   get(); // consume 'c'
   if(peek().c == 'p') {
      // cpus
      get();
      if(get().c != 'u')
         throw lexer_error(start, "Parser error: should be @cpus.");
      if(get().c != 's')
         throw lexer_error(start, "Parser error: should be @cpus.");
      return token(token::Token::CPUS, start, "@cpus");
   } else if(peek().c == 'l') {
      // cluster
      if(parse_word("luster"))
         return token(token::Token::CLUSTER, start, "@cluster");
      else
         throw lexer_error(start, "Parser error: should be @cluster.");
   } else
      throw lexer_error(start, "Parser error: must be either @cpus or @cluster.");
}

token
lexer::lex_delay(const character start)
{
   get();
   if(get().c != '+')
      throw lexer_error(start, "Parser error: expecting +TIMEs.");
   char max_string[32];
   size_t i(0);
   while(isdigit(peek().c) && i < 30)
      max_string[i++] = get().c;
   if(i == 0)
      throw lexer_error(start, "Parser error: expecting +TIMEs.");
   if(peek().c == 's') {
      max_string[i++] = get().c;
      max_string[i] = '\0';
      return token(token::Token::DELAY, start, max_string);
   } else if(peek().c == 'm') {
      max_string[i++] = get().c;
      if(get().c != 's')
         throw lexer_error(start, "Parser error: expecting +TIMEms.");
      max_string[i++] = 's';
      max_string[i] = '\0';
      return token(token::Token::DELAY, start, max_string);
   } else
      throw lexer_error(start, "Parser error: expecting +TIMEs.");
}

token
lexer::lex_at(const character start)
{
   switch(peek().c) {
      case 'a':
         return lex_arg_asc(start);
      case 'w':
         if(parse_word("world"))
            return token(token::Token::WORLD, start, "@world");
         else
            throw lexer_error(start, "Parser error: should be @world.");
      case 'c':
         return lex_cpus_cluster(start);
      case 'i':
         if(parse_word("initial"))
            return token(token::Token::INITIAL, start, "@initial");
         else
            throw lexer_error(start, "Parser error: should be @initial.");
      case 'd':
         if(parse_word("desc"))
            return token(token::Token::DESC, start, "@desc");
         else
            throw lexer_error(start, "Parser error: should be @desc.");
      case 't':
         if(parse_word("type"))
            return token(token::Token::ATTYPE, start, "@type");
         else
            throw lexer_error(start, "Parser error: should be @type.");
      case 'o':
         if(parse_word("order"))
            return token(token::Token::ORDER, start, "@order");
         else
            throw lexer_error(start, "Parser error: should be @order.");
      case 'r':
         if(parse_word("random"))
            return token(token::Token::ATRANDOM, start, "@random");
         else
            throw lexer_error(start, "Parser error: should be @random.");
      case '+':
         return lex_delay(start);
      case 's':
         if(parse_word("static"))
            return token(token::Token::STATIC, start, "@static");
         else
            throw lexer_error(start, "Parser error: should be @static.");
      default:
         if(isdigit(peek().c)) {
            char max_string[64];
            size_t i(0);
            max_string[i++] = get().c;
            while(isdigit(peek().c) && i < 64)
               max_string[i++] = get().c;
            max_string[i] = '\0';
            return token(token::Token::ADDRESS, start, max_string);
         } else
            throw lexer_error(start, "Cannot parse after @.");
   }
}

token
lexer::lex_filename(const character start)
{
   char max_string[1024];
   size_t i(0);
   while(peek().c != '\n' && i < 1024)
      max_string[i++] = get().c;
   if(i == 0)
      throw lexer_error(start, "Parser error: expecting #FILENAME.");
   return token(token::Token::FILENAME, start, max_string);
}

token
lexer::next_token(void)
{
   while(true) {
      character x(peek());
      if(x.c == 0)
         return token(token::Token::ENDFILE, x, ""); // eof
      else if(isspace(x.c)) {
         get();
         continue;
      } else
         break;
   }

   const character x(get());
   switch(x.c) {
      case ',': return token(token::Token::COMMA, x, ",");
      case '[': return token(token::Token::BRACKET_LEFT, x, string("["));
      case ']': return token(token::Token::BRACKET_RIGHT, x, string("]"));
      case '{': return token(token::Token::CURLY_LEFT, x, string("{"));
      case '}': return token(token::Token::CURLY_RIGHT, x, string("}"));
      case '.': return token(token::Token::DOT, x, string("."));
      case '"': return token(token::Token::STRING, x, lex_string());
      case '!': return token(token::Token::BANG, x, "!");
      case '?': return token(token::Token::QUESTION_MARK, x, "?");
      case '$': return token(token::Token::DOLLAR, x, "$");
      case '-': return lex_number_minus(x);
      case '=': return lex_equal(x);
      case ':': return lex_semicolon(x);
      case '(': return token(token::Token::PAREN_LEFT, x, "(");
      case ')': return token(token::Token::PAREN_RIGHT, x, ")");
      case '|': return lex_bar(x);
      case '&': return lex_and(x);
      case '/': return lex_slash(x);
      case '*': return token(token::Token::TIMES, x, "*");
      case '+': return token(token::Token::PLUS, x, "*");
      case '%': return token(token::Token::MOD, x, "%");
      case '<': return lex_lesser(x);
      case '_': return token(token::Token::ANONYMOUS_VAR, x, "_");
      case '@': return lex_at(x);
      case '#': return lex_filename(x);
      case 0: return token(token::Token::ENDFILE, x, ""); // eof
      default:
         if(isdigit(x.c))
            return lex_number(x);
         else if(islower(x.c))
            return lex_const(x);
         else if(isupper(x.c))
            return lex_variable(x);
         else
            throw lexer_error(x, string("Cannot identify next token " + utils::to_string(x.c)));
   }
}

lexer::lexer(const string& filename):
   stream(filename.c_str(), std::ios::in),
   cur_line(1), cur_col(0)
{
}

}
