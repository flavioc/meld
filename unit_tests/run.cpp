
#include <string>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include "mem/thread.hpp"
#include "vm/types.hpp"

using namespace std;
using namespace CppUnit;

#include "vm/all.hpp"
#include "vm/bitmap_tests.cpp"
#include "db/trie_tests.cpp"
#include "external/tests.cpp"

int
main(int argc, char **argv)
{
   (void)argc;
   (void)argv;
   mem::ensure_pool();
   vm::init_types();
   CppUnit::TextUi::TestRunner runner;
   CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
   runner.addTest(registry.makeTest());
   return runner.run("", false) ? 0 : 1;
}
