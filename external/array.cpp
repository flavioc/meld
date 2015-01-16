
#include "external/array.hpp"
#include "runtime/objs.hpp"
#include "vm/all.hpp"

using namespace std;
using namespace vm;

namespace vm
{
namespace external
{

argument
array_init(EXTERNAL_ARG(size), EXTERNAL_ARG(item), EXTERNAL_ARG(type))
{
   DECLARE_INT(size);
   DECLARE_ANY(item);
   DECLARE_INT(type);
   vm::type *typ(theProgram->get_type(type));

   runtime::array* x(runtime::array::create_fill(typ, size, item));
   RETURN_ARRAY(x);
}


}
}
