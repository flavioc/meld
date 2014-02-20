
#ifndef JIT_BUILD_HPP
#define JIT_BUILD_HPP

#include "conf.hpp"

#ifdef USE_JIT
#include <jit/jit.h>

#include "vm/instr.hpp"
#include "vm/state.hpp"

namespace jit
{

vm::pcounter compile_instruction(const vm::pcounter, vm::state&);
void compile_bytecode(vm::byte_code, const size_t, vm::state&);

}
#endif

#endif
