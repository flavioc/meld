#include <assert.h>

#include "db/tuple.hpp"

using namespace db;
using namespace vm;
using namespace std;

namespace db
{

void
simple_tuple::print(ostream& cout) const
{
   cout << *data << "@" << count;
}

void
aggregate_tuple::print(ostream& cout) const
{
   cout << *data << "@agg";
}

ostream& operator<<(ostream& cout, const stuple& tuple)
{
   tuple.print(cout);
   return cout;
}

}