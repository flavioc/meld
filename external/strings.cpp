
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
	DECLARE_ARG(s1, rstring::ptr);
	DECLARE_ARG(s2, rstring::ptr);
	
	const string s(s1->get_content() + s2->get_content());
	
	rstring::ptr result(rstring::make_string(s));
	
	RETURN_ARG(result);
}

}
	
}