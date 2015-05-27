
#include "external/core.hpp"

class ExternalTests : public TestFixture {
   public:

      void setUp(void)
      {
         vm::All = new vm::all();
         for(size_t i(0); i < 16; ++i)
            vm::All->SCHEDS.push_back((sched::thread*)i);
      }

      int partition_vertical(int x, int y, int lx, int ly)
      {
         vm::argument _x;
         _x.int_field = x;
         vm::argument _y;
         _y.int_field = y;
         vm::argument _lx;
         _lx.int_field = lx;
         vm::argument _ly;
         _ly.int_field = ly;
         vm::argument r = vm::external::partition_vertical(_x, _y, _lx, _ly);
         return (int)(r.ptr_field);
      }

      int partition_horizontal(int x, int y, int lx, int ly)
      {
         vm::argument _x;
         _x.int_field = x;
         vm::argument _y;
         _y.int_field = y;
         vm::argument _lx;
         _lx.int_field = lx;
         vm::argument _ly;
         _ly.int_field = ly;
         vm::argument r = vm::external::partition_horizontal(_x, _y, _lx, _ly);
         return (int)(r.ptr_field);
      }

      int partition_grid(int x, int y, int lx, int ly)
      {
         vm::argument _x;
         _x.int_field = x;
         vm::argument _y;
         _y.int_field = y;
         vm::argument _lx;
         _lx.int_field = lx;
         vm::argument _ly;
         _ly.int_field = ly;
         vm::argument r = vm::external::partition_grid(_x, _y, _lx, _ly);
         return (int)(r.ptr_field);
      }

      void testPartitionVertical(void)
      {
         vm::All->NUM_THREADS = 1;
         CPPUNIT_ASSERT(partition_vertical(0, 0, 1, 1) == 0);
         vm::All->NUM_THREADS = 2;
         CPPUNIT_ASSERT(partition_vertical(0, 0, 1, 1) == 0);
         CPPUNIT_ASSERT(partition_vertical(0, 0, 2, 1) == 0);
         CPPUNIT_ASSERT(partition_vertical(1, 0, 2, 1) == 1);
         vm::All->NUM_THREADS = 4;
         CPPUNIT_ASSERT(partition_vertical(0, 0, 4, 1) == 0);
         CPPUNIT_ASSERT(partition_vertical(1, 0, 4, 1) == 1);
         CPPUNIT_ASSERT(partition_vertical(2, 0, 4, 1) == 2);
         CPPUNIT_ASSERT(partition_vertical(3, 0, 4, 1) == 3);

         // 4 x 4: first row
         CPPUNIT_ASSERT(partition_vertical(0, 0, 4, 4) == 0);
         CPPUNIT_ASSERT(partition_vertical(0, 1, 4, 4) == 1);
         CPPUNIT_ASSERT(partition_vertical(0, 2, 4, 4) == 2);
         CPPUNIT_ASSERT(partition_vertical(0, 3, 4, 4) == 3);
         // 4 x 4: last row
         CPPUNIT_ASSERT(partition_vertical(3, 0, 4, 4) == 0);
         CPPUNIT_ASSERT(partition_vertical(3, 1, 4, 4) == 1);
         CPPUNIT_ASSERT(partition_vertical(3, 2, 4, 4) == 2);
         CPPUNIT_ASSERT(partition_vertical(3, 3, 4, 4) == 3);

         // 5 x 5: first row
         CPPUNIT_ASSERT(partition_vertical(4, 0, 5, 5) == 0);
         CPPUNIT_ASSERT(partition_vertical(4, 1, 5, 5) == 1);
         CPPUNIT_ASSERT(partition_vertical(4, 2, 5, 5) == 2);
         CPPUNIT_ASSERT(partition_vertical(4, 3, 5, 5) == 3);
         CPPUNIT_ASSERT(partition_vertical(4, 4, 5, 5) == 3);
         // 5 x 5: last row
         CPPUNIT_ASSERT(partition_vertical(4, 0, 5, 5) == 0);
         CPPUNIT_ASSERT(partition_vertical(4, 1, 5, 5) == 1);
         CPPUNIT_ASSERT(partition_vertical(4, 2, 5, 5) == 2);
         CPPUNIT_ASSERT(partition_vertical(4, 3, 5, 5) == 3);
         CPPUNIT_ASSERT(partition_vertical(4, 4, 5, 5) == 3);
      }

      void testPartitionHorizontal(void)
      {
         vm::All->NUM_THREADS = 1;
         // 1 x 1
         CPPUNIT_ASSERT(partition_horizontal(0, 0, 1, 1) == 0);
         vm::All->NUM_THREADS = 2;
         // 1 x 1
         CPPUNIT_ASSERT(partition_horizontal(0, 0, 1, 1) == 0);
         // 2 x 1
         CPPUNIT_ASSERT(partition_horizontal(0, 0, 2, 1) == 0);
         CPPUNIT_ASSERT(partition_horizontal(1, 0, 2, 1) == 1);
         // 3 x 1
         CPPUNIT_ASSERT(partition_horizontal(0, 0, 3, 1) == 0);
         CPPUNIT_ASSERT(partition_horizontal(1, 0, 3, 1) == 1);
         CPPUNIT_ASSERT(partition_horizontal(2, 0, 3, 1) == 1);
         // 1 x 3
         CPPUNIT_ASSERT(partition_horizontal(0, 0, 1, 3) == 0);
         CPPUNIT_ASSERT(partition_horizontal(0, 1, 1, 3) == 1);
         CPPUNIT_ASSERT(partition_horizontal(0, 2, 1, 3) == 1);
         vm::All->NUM_THREADS = 4;
         // 4 x 4
         // first row
         CPPUNIT_ASSERT(partition_horizontal(0, 0, 4, 4) == 0);
         CPPUNIT_ASSERT(partition_horizontal(0, 1, 4, 4) == 0);
         CPPUNIT_ASSERT(partition_horizontal(0, 2, 4, 4) == 0);
         CPPUNIT_ASSERT(partition_horizontal(0, 3, 4, 4) == 0);
         // first column
         CPPUNIT_ASSERT(partition_horizontal(0, 0, 4, 4) == 0);
         CPPUNIT_ASSERT(partition_horizontal(1, 0, 4, 4) == 1);
         CPPUNIT_ASSERT(partition_horizontal(2, 0, 4, 4) == 2);
         CPPUNIT_ASSERT(partition_horizontal(3, 0, 4, 4) == 3);
         // last column
         CPPUNIT_ASSERT(partition_horizontal(0, 3, 4, 4) == 0);
         CPPUNIT_ASSERT(partition_horizontal(1, 3, 4, 4) == 1);
         CPPUNIT_ASSERT(partition_horizontal(2, 3, 4, 4) == 2);
         CPPUNIT_ASSERT(partition_horizontal(3, 3, 4, 4) == 3);

         // 5 x 5
         // first row
         CPPUNIT_ASSERT(partition_horizontal(0, 0, 5, 5) == 0);
         CPPUNIT_ASSERT(partition_horizontal(0, 1, 5, 5) == 0);
         CPPUNIT_ASSERT(partition_horizontal(0, 2, 5, 5) == 0);
         CPPUNIT_ASSERT(partition_horizontal(0, 3, 5, 5) == 0);
         CPPUNIT_ASSERT(partition_horizontal(0, 4, 5, 5) == 0);
         // first column
         CPPUNIT_ASSERT(partition_horizontal(0, 0, 5, 5) == 0);
         CPPUNIT_ASSERT(partition_horizontal(1, 0, 5, 5) == 0);
         CPPUNIT_ASSERT(partition_horizontal(2, 0, 5, 5) == 1);
         CPPUNIT_ASSERT(partition_horizontal(3, 0, 5, 5) == 2);
         CPPUNIT_ASSERT(partition_horizontal(4, 0, 5, 5) == 3);
         // last column
         CPPUNIT_ASSERT(partition_horizontal(0, 4, 5, 5) == 0);
         CPPUNIT_ASSERT(partition_horizontal(1, 4, 5, 5) == 1);
         CPPUNIT_ASSERT(partition_horizontal(2, 4, 5, 5) == 2);
         CPPUNIT_ASSERT(partition_horizontal(3, 4, 5, 5) == 3);
         CPPUNIT_ASSERT(partition_horizontal(3, 4, 5, 5) == 3);
      }

      void testPartitionGrid(void)
      {
         vm::All->NUM_THREADS = 1;
         // 1 x 1
         CPPUNIT_ASSERT(partition_grid(0, 0, 1, 1) == 0);
         vm::All->NUM_THREADS = 2;
         // 1 x 1
         CPPUNIT_ASSERT(partition_grid(0, 0, 1, 1) == 0);
         // 1 x 2
         CPPUNIT_ASSERT(partition_grid(0, 0, 1, 2) == 0);
         CPPUNIT_ASSERT(partition_grid(0, 1, 1, 2) == 1);

         // 4 x 4
         vm::All->NUM_THREADS = 4;
         CPPUNIT_ASSERT(partition_grid(0, 0, 4, 4) == 0);
         CPPUNIT_ASSERT(partition_grid(1, 1, 4, 4) == 0);
         CPPUNIT_ASSERT(partition_grid(2, 2, 4, 4) == 2);
         CPPUNIT_ASSERT(partition_grid(3, 3, 4, 4) == 2);
         CPPUNIT_ASSERT(partition_grid(0, 3, 4, 4) == 1);
         CPPUNIT_ASSERT(partition_grid(3, 0, 4, 4) == 3);

         // 3 x 3
         CPPUNIT_ASSERT(partition_grid(0, 0, 3, 3) == 0);
         CPPUNIT_ASSERT(partition_grid(0, 1, 3, 3) == 1);
         CPPUNIT_ASSERT(partition_grid(0, 2, 3, 3) == 1);
         CPPUNIT_ASSERT(partition_grid(1, 0, 3, 3) == 3);
         CPPUNIT_ASSERT(partition_grid(1, 1, 3, 3) == 2);
         CPPUNIT_ASSERT(partition_grid(1, 2, 3, 3) == 2);
         CPPUNIT_ASSERT(partition_grid(2, 0, 3, 3) == 3);
         CPPUNIT_ASSERT(partition_grid(2, 1, 3, 3) == 2);
         CPPUNIT_ASSERT(partition_grid(2, 2, 3, 3) == 2);

         vm::All->NUM_THREADS = 5;
         CPPUNIT_ASSERT(partition_grid(0, 0, 3, 3) == 0);
         CPPUNIT_ASSERT(partition_grid(0, 1, 3, 3) == 1);
         CPPUNIT_ASSERT(partition_grid(2, 2, 3, 3) == 4);

         // 10 x 10
         CPPUNIT_ASSERT(partition_grid(9, 9, 10, 10) == 4);
         CPPUNIT_ASSERT(partition_grid(0, 9, 10, 10) == 1);
         CPPUNIT_ASSERT(partition_grid(9, 0, 10, 10) == 3);
         CPPUNIT_ASSERT(partition_grid(6, 9, 10, 10) == 4);

         vm::All->NUM_THREADS = 10;
         CPPUNIT_ASSERT(partition_grid(0, 0, 10, 10) == 0);
         CPPUNIT_ASSERT(partition_grid(9, 9, 10, 10) == 8);

         // 400 x 400
         CPPUNIT_ASSERT(partition_grid(0, 0, 400, 400) == 0);
         CPPUNIT_ASSERT(partition_grid(399, 399, 400, 400) == 7);
      }

      void tearDown(void)
      {
         delete vm::All;
      }

      CPPUNIT_TEST_SUITE(ExternalTests);

      CPPUNIT_TEST(testPartitionHorizontal);
      CPPUNIT_TEST(testPartitionVertical);
      CPPUNIT_TEST(testPartitionGrid);
      CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ExternalTests);
