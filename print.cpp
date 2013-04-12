
#include <cstdlib>
#include <iostream>

#include "vm/program.hpp"

using namespace vm;
using namespace process;
using namespace std;

int
main(int argc, char **argv)
{
   if(argc < 2) {
      fprintf(stderr, "usage: print <bytecode file>\n");
      return EXIT_FAILURE;
   }
   
   const string file(argv[1]);

   try {
      program prog(file);
      if(argc == 2)
         prog.print_bytecode(cout);
      else {
         for(int i(2); i < argc; ++i) {
            prog.print_bytecode_by_predicate(cout, string(argv[i]));
         }
      }
   } catch(vm::load_file_error& err) {
      cerr << "File error: " << err.what() << endl;
      exit(EXIT_FAILURE);
   }
   
   return EXIT_SUCCESS;
}
