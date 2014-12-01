#include "compiler/parser.hpp"

class ParserTests : public TestFixture {
   public:

      void setUp(void)
      {
      }

      void tearDown(void)
      {
      }

      void testParser()
      {
         using compiler::token;
         using compiler::parser;
         using compiler::lexer;

         parser p1("tests/progs/float.meld");
         p1.print(std::cout);

         parser p2("tests/progs/call-gc.meld");
         p2.print(std::cout);
      }

      CPPUNIT_TEST_SUITE(ParserTests);
      CPPUNIT_TEST(testParser);
      CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ParserTests);
