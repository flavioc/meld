
#include "vm/bitmap.hpp"

class VmBitmapTests : public TestFixture {
   public:

      vm::bitmap b;

      void setUp(void)
      {
         vm::bitmap::create(b, 2);
      }

      void tearDown(void)
      {
         vm::bitmap::destroy(b, 2);
      }

      void testBitmap(void)
      {
         CPPUNIT_ASSERT(b.empty(2));
         CPPUNIT_ASSERT(!b.get_bit(0));
         CPPUNIT_ASSERT(!b.get_bit(1));
         b.set_bit(0);
         CPPUNIT_ASSERT(b.get_bit(0));
         CPPUNIT_ASSERT(!b.get_bit(1));
         CPPUNIT_ASSERT(!b.empty(2));
         CPPUNIT_ASSERT(b.front(2) == 0);
         b.unset_bit(0);
         b.set_bit(3);
         CPPUNIT_ASSERT(b.front(2) == 3);
         CPPUNIT_ASSERT(!b.empty(2));
      }

      CPPUNIT_TEST_SUITE(VmBitmapTests);

      CPPUNIT_TEST(testBitmap);
      CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(VmBitmapTests);
