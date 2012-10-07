#include <assert.h>

#include "db/tuple.hpp"

using namespace db;
using namespace vm;
using namespace std;
using namespace boost;
using namespace utils;

namespace db
{

#ifdef COMPILE_MPI
void
simple_tuple::pack(byte *buf, const size_t buf_size, int *pos, MPI_Comm comm) const
{
   MPI_Pack((void*)&count, 1, MPI_SHORT, buf, buf_size, pos, comm);
   
   data->pack(buf, buf_size, pos, comm);
}

simple_tuple*
simple_tuple::unpack(byte *buf, const size_t buf_size, int *pos, MPI_Comm comm)
{
   ref_count count;
   
   MPI_Unpack(buf, buf_size, pos, &count, 1, MPI_SHORT, comm);
   
   vm::tuple *tpl(vm::tuple::unpack(buf, buf_size, pos, comm));
   
   return new simple_tuple(tpl, count);
}
#endif

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
}

ostream& operator<<(ostream& cout, const simple_tuple& tuple)
{
   tuple.print(cout);
   return cout;
}

}