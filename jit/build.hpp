
#ifndef JIT_BUILD_HPP
#define JIT_BUILD_HPP

#include <jit/jit.h>

#include "vm/instr.hpp"
#include "vm/state.hpp"

namespace jit
{

void jit_compile(vm::byte_code, const size_t);

}

#endif
