
#ifndef RUNTIME_REF_BASE_HPP
#define RUNTIME_REF_BASE_HPP

#include <atomic>

#include "vm/defs.hpp"
#include "mem/base.hpp"

namespace runtime
{

/* we assume that reference types (lists, structs, strings) have a reference counter at the beginning of the memory object */
struct ref_base
{
public:
	std::atomic<vm::ref_count> refs;
};

};

#endif
