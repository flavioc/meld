
#include "compiler/lexer.hpp"

using std::string;
using std::ifstream;
using std::make_pair;
using std::pair;

namespace compiler
{

token token::generated("generated");

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
      throw lexer_error(start, "Cannot identify next token. Did you mean a-b? (lex_number)");

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
         return token(token::Token::NUMBER, filename, start, max_string);
      } else {
         max_string[i++] = '.';
         i += floats;
         max_string[i++] = '\0';
         return token(token::Token::NUMBER, filename, start, max_string);
      }
   }

   max_string[i++] = '\0';
   return token(token::Token::NUMBER, filename, start, max_string);
}

token
lexer::lex_number_minus(const character start)
{
   character next(peek());
   if(isend(next)) {
      get();
      return token(token::Token::MINUS, filename, start, "-");
   }

   if(next.c == 'o') {
      get();
      return token(token::Token::LOLLI, filename, start, "-o");
   }

   return lex_number(start);
}

token
lexer::lex_equal(const character start)
{
   if(peek().c == '>') {
      get();
      return token(token::Token::ARROW, filename, start, "=>");
   }

   return token(token::Token::EQUAL, filename, start, "=");
}

token
lexer::lex_semicolon(const character start)
{
   if(peek().c == '(') {
      get();
      return token(token::Token::LPACO, filename, start, ":(");
   }
   if(peek().c == ':') {
      get();
      return token(token::Token::PROLOG_LOLLI, filename, start, ":-");
   }
   return token(token::Token::SEMICOLON, filename, start, ":");
}

token
lexer::lex_bar(const character start)
{
   if(peek().c == '|') {
      get();
      return token(token::Token::OR, filename, start, "||");
   }
   return token(token::Token::BAR, filename, start, "|");
}

token
lexer::lex_and(const character start)
{
   if(get().c != '&')
      throw lexer_error(start, "cannot identify next token. Did you mean &&?");
   return token(token::Token::AND, filename, start, "&&");
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
               return token(token::Token::ENDFILE, filename, start, "");

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
         return token(token::Token::DIVIDE, filename, start, "/");
   }
}

token
lexer::lex_lesser(const character start)
{
   switch(peek().c) {
      case '=': get(); return token(token::Token::LESSER_EQUAL, filename, start, "<=");
      case '>': get(); return token(token::Token::NOT_EQUAL, filename, start, "<>");
      default: return token(token::Token::LESSER, filename, start, "<");
   }
}

token
lexer::lex_greater(const character start)
{
   switch(peek().c) {
      case '=': get(); return token(token::Token::GREATER_EQUAL, filename, start, ">=");
      default: return token(token::Token::GREATER, filename, start, ">");
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
         return token(token::Token::VAR, filename, start, max_string);
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
            return token(token::Token::TYPE, filename, start, name);
         else if(name == "exists")
            return token(token::Token::EXISTS, filename, start, name);
         else if(name == "extern")
            return token(token::Token::EXTERN, filename, start, name);
         else if(name == "const")
            return token(token::Token::CONST, filename, start, name);
         else if(name == "bool")
            return token(token::Token::BOOL, filename, start, name);
         else if(name == "int")
            return token(token::Token::INT, filename, start, name);
         else if(name == "float")
            return token(token::Token::FLOAT, filename, start, name);
         else if(name == "thread")
            return token(token::Token::THREAD, filename, start, name);
         else if(name == "node")
            return token(token::Token::NODE, filename, start, name);
         else if(name == "string")
            return token(token::Token::STRING, filename, start, name);
         else if(name == "list")
            return token(token::Token::LIST, filename, start, name);
         else if(name == "include")
            return token(token::Token::INCLUDE, filename, start, name);
         else if(name == "random")
            return token(token::Token::RANDOM, filename, start, name);
         else if(name == "route")
            return token(token::Token::ROUTE, filename, start, name);
         else if(name == "action")
            return token(token::Token::ACTION, filename, start, name);
         else if(name == "linear")
            return token(token::Token::LINEAR, filename, start, name);
         else if(name == "export")
            return token(token::Token::EXPORT, filename, start, name);
         else if(name == "import")
            return token(token::Token::IMPORT, filename, start, name);
         else if(name == "from")
            return token(token::Token::FROM, filename, start, name);
         else if(name == "as")
            return token(token::Token::AS, filename, start, name);
         else if(name == "let")
            return token(token::Token::LET, filename, start, name);
         else if(name == "in")
            return token(token::Token::IN, filename, start, name);
         else if(name == "end")
            return token(token::Token::END, filename, start, name);
         else if(name == "if")
            return token(token::Token::IF, filename, start, name);
         else if(name == "then")
            return token(token::Token::THEN, filename, start, name);
         else if(name == "else")
            return token(token::Token::ELSE, filename, start, name);
         else if(name == "otherwise")
            return token(token::Token::OTHERWISE, filename, start, name);
         else if(name == "fun")
            return token(token::Token::FUN, filename, start, name);
         else if(name == "min")
            return token(token::Token::MIN, filename, start, name);
         else if(name == "priority")
            return token(token::Token::PRIORITY, filename, start, name);
         else if(name == "true")
            return token(token::Token::TRUE, filename, start, name);
         else if(name == "false")
            return token(token::Token::FALSE, filename, start, name);
         else if(name == "nil")
            return token(token::Token::NIL, filename, start, name);
         else
            return token(token::Token::NAME, filename, start, name);
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
            return token(token::Token::ARG, filename, start, max_string);
         }
      }
   } else if(peek().c == 's') {
      get();
      if(get().c != 'c')
         throw lexer_error(start, "Parser error: should be @asc.");
      return token(token::Token::ASC, filename, start, "@asc");
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
      return token(token::Token::CPUS, filename, start, "@cpus");
   } else if(peek().c == 'l') {
      // cluster
      if(parse_word("luster"))
         return token(token::Token::CLUSTER, filename, start, "@cluster");
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
      return token(token::Token::DELAY, filename, start, max_string);
   } else if(peek().c == 'm') {
      max_string[i++] = get().c;
      if(get().c != 's')
         throw lexer_error(start, "Parser error: expecting +TIMEms.");
      max_string[i++] = 's';
      max_string[i] = '\0';
      return token(token::Token::DELAY, filename, start, max_string);
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
            return token(token::Token::WORLD, filename, start, "@world");
         else
            throw lexer_error(start, "Parser error: should be @world.");
      case 'c':
         return lex_cpus_cluster(start);
      case 'i':
         if(parse_word("initial"))
            return token(token::Token::INITIAL, filename, start, "@initial");
         else
            throw lexer_error(start, "Parser error: should be @initial.");
      case 'd':
         if(parse_word("desc"))
            return token(token::Token::DESC, filename, start, "@desc");
         else
            throw lexer_error(start, "Parser error: should be @desc.");
      case 't':
         if(parse_word("type"))
            return token(token::Token::ATTYPE, filename, start, "@type");
         else
            throw lexer_error(start, "Parser error: should be @type.");
      case 'o':
         if(parse_word("order"))
            return token(token::Token::ORDER, filename, start, "@order");
         else
            throw lexer_error(start, "Parser error: should be @order.");
      case 'r':
         if(parse_word("random"))
            return token(token::Token::ATRANDOM, filename, start, "@random");
         else
            throw lexer_error(start, "Parser error: should be @random.");
      case '+':
         return lex_delay(start);
      case 's':
         if(parse_word("static"))
            return token(token::Token::STATIC, filename, start, "@static");
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
            return token(token::Token::ADDRESS, filename, start, max_string);
         } else
            throw lexer_error(start, "Cannot parse after @.");
   }
}

token
lexer::lex_filename(const character start)
{
   char max_string[1024];
   size_t i(0);
   while(!isspace(peek().c) && i < 1024)
      max_string[i++] = get().c;
   if(i == 0)
      throw lexer_error(start, "Parser error: expecting #FILENAME.");
   max_string[i] = '\0';
   return token(token::Token::FILENAME, filename, start, max_string);
}

token
lexer::next_token(void)
{
   while(true) {
      character x(peek());
      if(x.c == 0)
         return token(token::Token::ENDFILE, filename, x, ""); // eof
      else if(isspace(x.c)) {
         get();
         continue;
      } else
         break;
   }

   const character x(get());
   switch(x.c) {
      case ',': return token(token::Token::COMMA, filename, x, ",");
      case '[': return token(token::Token::BRACKET_LEFT, filename, x, string("["));
      case ']': return token(token::Token::BRACKET_RIGHT, filename, x, string("]"));
      case '{': return token(token::Token::CURLY_LEFT, filename, x, string("{"));
      case '}': return token(token::Token::CURLY_RIGHT, filename, x, string("}"));
      case '.': return token(token::Token::DOT, filename, x, string("."));
      case '"': return token(token::Token::STRING, filename, x, lex_string());
      case '!': return token(token::Token::BANG, filename, x, "!");
      case '?': return token(token::Token::QUESTION_MARK, filename, x, "?");
      case '$': return token(token::Token::DOLLAR, filename, x, "$");
      case '-': return lex_number_minus(x);
      case '=': return lex_equal(x);
      case ':': return lex_semicolon(x);
      case '(': return token(token::Token::PAREN_LEFT, filename, x, "(");
      case ')': return token(token::Token::PAREN_RIGHT, filename, x, ")");
      case '|': return lex_bar(x);
      case '&': return lex_and(x);
      case '/': return lex_slash(x);
      case '*': return token(token::Token::TIMES, filename, x, "*");
      case '+': return token(token::Token::PLUS, filename, x, "*");
      case '%': return token(token::Token::MOD, filename, x, "%");
      case '<': return lex_lesser(x);
      case '>': return lex_greater(x);
      case '_': return token(token::Token::ANONYMOUS_VAR, filename, x, "_");
      case '@': return lex_at(x);
      case '#': return lex_filename(x);
      case 0: return token(token::Token::ENDFILE, filename, x, ""); // eof
      default:
         if(isdigit(x.c))
            return lex_number(x);
         else if(islower(x.c))
            return lex_const(x);
         else if(isupper(x.c))
            return lex_variable(x);
         else
            throw lexer_error(x, string("cannot identify next token " + utils::to_string(x.c)));
   }
}

lexer::lexer(const string& filename):
   filename(filename),
   stream(filename.c_str(), std::ios::in),
   cur_line(1), cur_col(0)
{
}

}
