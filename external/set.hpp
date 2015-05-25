
#ifndef EXTERNAL_SET_HPP
#define EXTERNAL_SET_HPP

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "vm/external.hpp"

namespace vm
{

namespace external
{

argument set_size(const argument);
argument set_exists(const argument, const argument, const argument);
argument set_add(const argument, const argument, const argument);
argument set_from_list(const argument, const argument);

}

}

#endif
