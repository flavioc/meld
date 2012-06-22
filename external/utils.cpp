
#include "external/utils.hpp"
#include "utils/utils.hpp"
#include "runtime/string.hpp"
#include "utils/utils.hpp"

namespace vm
{
namespace external
{

using namespace runtime;
using namespace utils;
using namespace std;

argument
randint(EXTERNAL_ARG(x))
{
   DECLARE_ARG(x, int_val);
   
   const int_val ret((int_val)utils::random_unsigned((size_t)x));
   
   RETURN_ARG(ret);
}   

argument
str2float(EXTERNAL_ARG(x))
{
	DECLARE_ARG(x, rstring::ptr);
	
	const string str(x->get_content());
	float_val f;

	from_string(f, str, std::dec);


	RETURN_ARG(f);
}

}
}
