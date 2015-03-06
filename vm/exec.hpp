
#ifndef VM_EXEC_HPP
#define VM_EXEC_HPP

#include <memory>
#include <stdexcept>
#include <vector>
#include <strstream>

#include "vm/state.hpp"
#include "vm/defs.hpp"
#include "vm/instr.hpp"
#include "incbin.h"

namespace vm {

typedef enum { EXECUTION_OK, EXECUTION_CONSUMED } execution_return;

execution_return execute_process(byte_code, state&, vm::tuple*, vm::predicate*);
void execute_rule(const rule_id, state&);

class vm_exec_error : public std::runtime_error {
   public:
   explicit vm_exec_error(const std::string& msg) : std::runtime_error(msg) {}
};
}

#ifdef COMPILED
std::unique_ptr<std::istrstream> compiled_database_stream();
#endif

#endif
