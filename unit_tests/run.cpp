
#include <string>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace std;
using namespace CppUnit;

#include "vm/all.hpp"
#include "external/tests.cpp"

int
main(int argc, char **argv)
{
   (void)argc;
   (void)argv;
   CppUnit::TextUi::TestRunner runner;
   CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
   runner.addTest(registry.makeTest());
   return runner.run("", false) ? 0 : 1;
}
