#include <assert.h>

#include "vm/full_tuple.hpp"

using namespace std;
using namespace utils;

namespace vm
{

void
full_tuple::pack(byte *buf, const size_t buf_size, int *pos) const
{
   utils::pack<derivation_count>((void*)&count, 1, buf, buf_size, pos);
   utils::pack<depth_t>((void*)&depth, 1, buf, buf_size, pos);
   
   data->pack(get_predicate(), buf, buf_size, pos);
}

full_tuple*
full_tuple::unpack(vm::predicate *pred, byte *buf, const size_t buf_size, int *pos, vm::program *prog)
{
   derivation_count count;
   depth_t depth;
   
   utils::unpack<derivation_count>(buf, buf_size, pos, &count, 1);
   utils::unpack<vm::depth_t>(buf, buf_size, pos, &depth, 1);
   
   vm::tuple *tpl(vm::tuple::unpack(buf, buf_size, pos, prog));
   
   return new full_tuple(tpl, pred, count, depth);
}

full_tuple::~full_tuple(void)
{
   // full_tuple is not allowed to delete tuples
}

void
full_tuple::print(ostream& cout) const
{
   data->print(cout, get_predicate());
	
	if(count > 1)
		cout << "@" << count;
   if(count < 0)
      cout << "@" << count;
}

ostream& operator<<(ostream& cout, const full_tuple& tuple)
{
   tuple.print(cout);
   return cout;
}

}
