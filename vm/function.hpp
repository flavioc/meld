
#ifndef VM_FUNCTION_HPP
#define VM_FUNCTION_HPP

#include "conf.hpp"
#include "vm/defs.hpp"

namespace vm
{

class function
{
   private:

      byte_code code;
		code_size_t code_size;

   public:

      byte_code get_bytecode(void) const { return code; }
      code_size_t get_bytecode_size(void) const { return code_size; }

      explicit function(byte_code _code, code_size_t _size):
         code(_code), code_size(_size)
      {}

      ~function(void)
      {
         delete []code;
      }
};

}

#endif
