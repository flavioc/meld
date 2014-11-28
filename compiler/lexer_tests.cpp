
#include "compiler/lexer.hpp"

class LexerTests : public TestFixture {
   public:

      compiler::lexer *lex;

      void setUp(void)
      {
         lex = new compiler::lexer("tests/progs/float.meld");
      }

      void tearDown(void)
      {

      }

      void check(compiler::token::Token typ)
      {
         compiler::token tok(lex->next_token());
         CPPUNIT_ASSERT(tok.tok == typ);
      }

      void testLexer()
      {
         using compiler::token;

         check(token::Token::TYPE);
         check(token::Token::LINEAR);
         check(token::Token::NAME);
         check(token::Token::PAREN_LEFT);
         check(token::Token::NODE);
         check(token::Token::COMMA);
         check(token::Token::FLOAT);
         check(token::Token::COMMA);
         check(token::Token::INT);
         check(token::Token::PAREN_RIGHT);
         check(token::Token::DOT);
         check(token::Token::TYPE);

         check(token::Token::LINEAR);
         check(token::Token::NAME);
         check(token::Token::PAREN_LEFT);
         check(token::Token::NODE);
         check(token::Token::COMMA);
         check(token::Token::FLOAT);
         check(token::Token::PAREN_RIGHT);
         check(token::Token::DOT);

         check(token::Token::NAME);
         check(token::Token::PAREN_LEFT);
         check(token::Token::ADDRESS);
         check(token::Token::COMMA);
         check(token::Token::NUMBER);
         check(token::Token::COMMA);
         check(token::Token::NUMBER);
         check(token::Token::PAREN_RIGHT);
         check(token::Token::DOT);

         check(token::Token::NAME);
         check(token::Token::PAREN_LEFT);
         check(token::Token::VAR);
         check(token::Token::COMMA);
         check(token::Token::VAR);
         check(token::Token::COMMA);
         check(token::Token::VAR);
         check(token::Token::PAREN_RIGHT);
         check(token::Token::LOLLI);
         check(token::Token::NAME);
         check(token::Token::PAREN_LEFT);
         check(token::Token::VAR);
         check(token::Token::COMMA);
         check(token::Token::VAR);
         check(token::Token::DIVIDE);
         check(token::Token::FLOAT);
         check(token::Token::PAREN_LEFT);
         check(token::Token::VAR);
         check(token::Token::PAREN_RIGHT);
         check(token::Token::PAREN_RIGHT);
         check(token::Token::DOT);
         check(token::Token::ENDFILE);
      }

      CPPUNIT_TEST_SUITE(LexerTests);
      CPPUNIT_TEST(testLexer);
      CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(LexerTests);
