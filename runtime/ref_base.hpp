
#ifndef RUNTIME_REF_BASE_HPP
#define RUNTIME_REF_BASE_HPP

#include "utils/atomic.hpp"
#include "vm/defs.hpp"
#include "mem/base.hpp"

namespace runtime
{

/* we assume that reference types (lists, structs, strings) have a reference counter at the beginning of the memory object */
class ref_base: public mem::base
{
public:
	utils::atomic<vm::ref_count> refs;
};

};

#endif
