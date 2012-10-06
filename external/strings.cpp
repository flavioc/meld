
#include <string>

#include "external/strings.hpp"
#include "runtime/string.hpp"

namespace vm
{
	
namespace external
{
	
using namespace runtime;
using namespace std;
	
argument
concatenate(EXTERNAL_ARG(s1), EXTERNAL_ARG(s2))
{
	DECLARE_STRING(s1);
	DECLARE_STRING(s2);
	
	const string s(s1->get_content() + s2->get_content());
	
	rstring::ptr result(rstring::make_string(s));
	
	RETURN_STRING(result);
}

}
	
}
