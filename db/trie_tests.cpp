
#include "db/trie.hpp"

class DbTrieTests : public TestFixture {
   public:

      vm::predicate *f;
      db::tuple_trie *trie;

      void setUp(void)
      {
         std::vector<vm::type*> types;
         types.push_back(vm::TYPE_INT);
         types.push_back(vm::TYPE_INT);
         CPPUNIT_ASSERT(vm::TYPE_INT != nullptr);
         f = vm::predicate::make_predicate_simple(0, "f", true, types);
         CPPUNIT_ASSERT(f->num_fields() == 2);
         trie = new db::tuple_trie();
      }

      void tearDown(void)
      {

      }

      void testTrie(void)
      {
         mem::node_allocator alloc;
         // insert three tuples

         CPPUNIT_ASSERT(trie->size() == 0);
         CPPUNIT_ASSERT(trie->begin() == trie->end());
         vm::tuple *f1(vm::tuple::create(f, &alloc));
         f1->set_int(0, 1);
         f1->set_int(1, 2);
         CPPUNIT_ASSERT(f1);

         CPPUNIT_ASSERT(trie->insert_tuple(f1, f));
         CPPUNIT_ASSERT(trie->size() == 1);

         auto it(trie->begin());
         CPPUNIT_ASSERT(it != trie->end());
         it++;
         CPPUNIT_ASSERT(it == trie->end());

         CPPUNIT_ASSERT(!trie->insert_tuple(f1, f));
         CPPUNIT_ASSERT(!trie->insert_tuple(f1, f));
         CPPUNIT_ASSERT(!trie->insert_tuple(f1, f));
         CPPUNIT_ASSERT(!trie->insert_tuple(f1, f));
         CPPUNIT_ASSERT(trie->size() == 5);

         vm::tuple *f2(vm::tuple::create(f, &alloc));
         f2->set_int(0, 1);
         f2->set_int(1, 3);
         CPPUNIT_ASSERT(trie->insert_tuple(f2, f));
         CPPUNIT_ASSERT(!trie->insert_tuple(f2, f));
         CPPUNIT_ASSERT(trie->size() == 7);

         vm::tuple *f3(vm::tuple::create(f, &alloc));
         f3->set_int(0, 2);
         f3->set_int(1, 3);
         CPPUNIT_ASSERT(trie->insert_tuple(f3, f));
         CPPUNIT_ASSERT(!trie->insert_tuple(f3, f));
         CPPUNIT_ASSERT(trie->size() == 9);

         // delete all
         db::trie::delete_info inf1(trie->delete_tuple(f3, f));
         CPPUNIT_ASSERT(inf1.is_valid());
         CPPUNIT_ASSERT(!inf1.to_delete());
         CPPUNIT_ASSERT(trie->size() == 8);

         db::trie::delete_info inf2(trie->delete_tuple(f3, f));
         CPPUNIT_ASSERT(inf2.is_valid());
         CPPUNIT_ASSERT(inf2.to_delete());
         vm::candidate_gc_nodes gc_nodes;
         inf2.perform_delete(f, &alloc, gc_nodes);
         CPPUNIT_ASSERT(trie->size() == 7);

         db::trie::delete_info inf3(trie->delete_tuple(f2, f));
         CPPUNIT_ASSERT(inf3.is_valid());
         CPPUNIT_ASSERT(!inf3.to_delete());
         CPPUNIT_ASSERT(trie->size() == 6);

         db::trie::delete_info inf4(trie->delete_tuple(f2, f));
         CPPUNIT_ASSERT(inf4.is_valid());
         CPPUNIT_ASSERT(inf4.to_delete());
         inf4.perform_delete(f, &alloc, gc_nodes);
         CPPUNIT_ASSERT(trie->size() == 5);

         db::trie::delete_info inf5(trie->delete_tuple(f1, f));
         CPPUNIT_ASSERT(inf5.is_valid());
         CPPUNIT_ASSERT(!inf5.to_delete());
         CPPUNIT_ASSERT(trie->size() == 4);
         db::trie::delete_info inf6(trie->delete_tuple(f1, f));
         CPPUNIT_ASSERT(inf6.is_valid());
         CPPUNIT_ASSERT(!inf6.to_delete());
         CPPUNIT_ASSERT(trie->size() == 3);
         db::trie::delete_info inf7(trie->delete_tuple(f1, f));
         CPPUNIT_ASSERT(inf7.is_valid());
         CPPUNIT_ASSERT(!inf7.to_delete());
         CPPUNIT_ASSERT(trie->size() == 2);
         db::trie::delete_info inf8(trie->delete_tuple(f1, f));
         CPPUNIT_ASSERT(inf8.is_valid());
         CPPUNIT_ASSERT(!inf8.to_delete());
         CPPUNIT_ASSERT(trie->size() == 1);

         db::trie::delete_info inf9(trie->delete_tuple(f1, f));
         CPPUNIT_ASSERT(inf9.is_valid());
         CPPUNIT_ASSERT(inf9.to_delete());
         inf9.perform_delete(f, &alloc, gc_nodes);
         CPPUNIT_ASSERT(trie->size() == 0);
         CPPUNIT_ASSERT(trie->empty());
      }

      CPPUNIT_TEST_SUITE(DbTrieTests);

      CPPUNIT_TEST(testTrie);
      CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DbTrieTests);
