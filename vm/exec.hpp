
#ifndef VM_EXEC_HPP
#define VM_EXEC_HPP

#include <stdexcept>
#include <vector>

#include "vm/state.hpp"
#include "vm/defs.hpp"
#include "vm/instr.hpp"

namespace vm
{
   
typedef enum {
   EXECUTION_OK,
   EXECUTION_CONSUMED
} execution_return;

execution_return execute_bytecode(byte_code, state&);
void execute_rule(const rule_id, state&);

class vm_exec_error : public std::runtime_error {
 public:
    explicit vm_exec_error(const std::string& msg) :
         std::runtime_error(msg)
    {}
};

}

#endif
