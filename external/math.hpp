
#ifndef EXTERNAL_MATH_HPP
#define EXTERNAL_MATH_HPP

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "vm/external.hpp"

namespace vm
{
   
namespace external
{
   
argument sigmoid(const argument);
argument normalize(const argument);
argument damp(const argument, const argument, const argument);
argument divide(const argument, const argument);
argument convolve(const argument, const argument);
argument addfloatlists(const argument, const argument);
argument residual(const argument, const argument);
argument intpower(const argument, const argument);

}

}

#endif

