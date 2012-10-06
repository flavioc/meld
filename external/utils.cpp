
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
   DECLARE_INT(x);
   
   const int_val ret((int_val)utils::random_unsigned((size_t)x));
   
   RETURN_INT(ret);
}   

argument
str2float(EXTERNAL_ARG(x))
{
	DECLARE_STRING(x);
	
	const string str(x->get_content());
	float_val f;

	from_string(f, str, std::dec);

	RETURN_FLOAT(f);
}

argument
str2int(EXTERNAL_ARG(x))
{
	DECLARE_STRING(x);

	const string str(x->get_content());
	int_val i;

	from_string(i, str, std::dec);

	RETURN_INT(i);
}

argument
wastetime(EXTERNAL_ARG(x))
{
	DECLARE_INT(x);

	usleep(x * 1000);

	RETURN_INT(x);
}

}
}
