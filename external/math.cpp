
#include <cmath>

#include "external/math.hpp"

namespace vm
{
namespace external
{
   
argument
sigmoid(EXTERNAL_ARG(x))
{
   DECLARE_ARG(x, float_val);
   
   const float_val ret(1.0/(1.0 + exp(-x)));
   
   RETURN_ARG(ret);
}

}
}