#include <assert.h>

#include "db/tuple.hpp"

using namespace db;
using namespace vm;
using namespace std;
using namespace boost;
using namespace utils;

namespace db
{

void
simple_tuple::pack(byte *buf, const size_t buf_size, int *pos) const
{
   utils::pack<short>((void*)&count, 1, buf, buf_size, pos);
   
   data->pack(buf, buf_size, pos);
}

simple_tuple*
simple_tuple::unpack(byte *buf, const size_t buf_size, int *pos, vm::program *prog)
{
   ref_count count;
   
   utils::unpack<short>(buf, buf_size, pos, &count, 1);
   
   vm::tuple *tpl(vm::tuple::unpack(buf, buf_size, pos, prog));
   
   return new simple_tuple(tpl, count);
}

simple_tuple::~simple_tuple(void)
{
   // simple_tuple is not allowed to delete tuples
}

void
simple_tuple::print(ostream& cout) const
{
	cout << *data;
	
	if(count > 1)
		cout << "@" << count;
   if(count < 0)
      cout << "@" << count;
}

ostream& operator<<(ostream& cout, const simple_tuple& tuple)
{
   tuple.print(cout);
   return cout;
}

}
