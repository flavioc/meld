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
         parser p2("tests/progs/visit.meld");
         p2.print(std::cout);
         parser p3("tests/progs/list-floats.meld");
         p3.print(std::cout);
         parser p4("tests/progs/binary.meld");
         p4.print(std::cout);
         parser p5("tests/progs/constant.meld");
         p5.print(std::cout);
         parser p6("tests/progs/bipartite.meld");
         p6.print(std::cout);
         parser p7("tests/progs/include.meld");
         p7.print(std::cout);
      }

      CPPUNIT_TEST_SUITE(ParserTests);
      CPPUNIT_TEST(testParser);
      CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ParserTests);
