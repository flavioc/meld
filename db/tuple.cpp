#include <assert.h>

#include "db/tuple.hpp"

using namespace db;
using namespace vm;
using namespace std;
using namespace boost;

namespace db
{
   
void
simple_tuple::save(mpi::packed_oarchive& ar, const unsigned int version) const
{
   ar & *data;
   ar & count;
}

void
simple_tuple::load(mpi::packed_iarchive& ar, const unsigned int version)
{
   data = new vm::tuple();
   
   ar & *data;
   ar & count;
}

simple_tuple::~simple_tuple(void)
{
}

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