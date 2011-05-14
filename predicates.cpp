
#include <cstdlib>
#include <iostream>

#include "vm/program.hpp"

using namespace vm;
using namespace process;
using namespace std;

int
main(int argc, char **argv)
{
   if(argc != 2) {
      fprintf(stderr, "usage: predicates <bytecode file>\n");
      return EXIT_FAILURE;
   }
   
   const string file(argv[1]);
   
   program prog(file);
   
   prog.print_predicates(cout);
   
   return EXIT_SUCCESS;
}
