
#include <iostream>

#include "utils/types.hpp"

using namespace std;

namespace utils
{

void print_byte(byte b)
{
   for(size_t i(0); i < 8*sizeof(byte); ++i) {
      cout << (b & 0x80 ? 1 : 0);
      b = b << 1;
   }
}

void print_uint64_t(uint64_t u)
{
   byte *b((byte*)&u);
   for(size_t i(0); i < sizeof(uint64_t); ++i) {
      print_byte(b[i]);
   }
}

}
