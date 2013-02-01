
#include "external/core.hpp"

namespace vm
{
namespace external
{

argument
node_priority(EXTERNAL_ARG(id))
{
   float_val ret(0.0);

   RETURN_FLOAT(ret);
}

}
}
