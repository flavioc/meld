
#include "external/utils.hpp"
#include "utils/utils.hpp"

namespace vm
{
namespace external
{

argument
randint(EXTERNAL_ARG(x))
{
   DECLARE_ARG(x, int_val);
   
   const int_val ret((int_val)utils::random_unsigned((size_t)x));
   
   RETURN_ARG(ret);
}   

}
}