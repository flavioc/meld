
#ifndef RUNTIME_COMMON_HPP
#define RUNTIME_COMMON_HPP

#include "vm/types.hpp"
#include "vm/defs.hpp"

namespace runtime
{

void increment_runtime_data(const vm::tuple_field&, vm::type *);
void decrement_runtime_data(const vm::tuple_field&, vm::type *);

}

#endif
