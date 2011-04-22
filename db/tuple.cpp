#include <assert.h>

#include "db/tuple.hpp"

using namespace db;
using namespace vm;
using namespace std;
using namespace boost;

namespace db
{

#ifdef COMPILE_MPI
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
#endif

simple_tuple::~simple_tuple(void)
{
   // simple_tuple is not allowed to delete tuples
}

void
simple_tuple::print(ostream& cout) const
{
   cout << *data << "@" << count;
}

ostream& operator<<(ostream& cout, const simple_tuple& tuple)
{
   tuple.print(cout);
   return cout;
}

void
agg_configuration::add_to_set(vm::tuple *tpl, const ref_count many)
{
   count += many;
   values.push_back(tpl);
}

const bool
agg_configuration::test(tuple *tpl, const field_num agg_field) const
{
   tuple_list::const_iterator it(values.begin());
   tuple *other(*it);
   const predicate *pred(tpl->get_predicate());
   
   for(field_num i(0); i < pred->num_fields(); ++i)
      if(i != agg_field && !tpl->field_equal(*other, i))
         return false;
   
   return true;
}

tuple*
agg_configuration::generate_max_int(const field_num field)
{
   tuple_list::iterator it(values.begin());
   int_val max_val = (*it)->get_int(field);
   tuple_list::iterator sit = it;
   
   ++it;
   for(; it != values.end(); ++it)
   {
      tuple *other(*it);
      
      if(max_val < other->get_int(field)) {
         max_val = other->get_int(field);
         sit = it;
      }
   }
   
   tuple *ret(*sit);
   values.erase(sit);
   
   return ret;
}

tuple*
agg_configuration::generate_min_int(const field_num field)
{
   tuple_list::iterator it(values.begin());
   int_val min_val = (*it)->get_int(field);
   tuple_list::iterator sit = it;
   
   ++it;
   for(; it != values.end(); ++it) {
      tuple *other(*it);
      
      if(min_val > other->get_int(field)) {
         min_val = other->get_int(field);
         sit = it;
      }
   }
   
   tuple *ret(*sit);
   values.erase(sit);
   
   return ret;
}

tuple*
agg_configuration::generate_sum_int(const field_num field)
{
   tuple_list::iterator it(values.begin());
   int_val sum_val = 0;
   tuple *ret(*it);
   
   for(; it != values.end(); ++it)
      sum_val += (*it)->get_int(field);
   
   values.pop_front();
   
   ret->set_int(field, sum_val);
   
   return ret;
}

tuple*
agg_configuration::generate_sum_float(const field_num field)
{
   tuple_list::iterator it(values.begin());
   float_val sum_val = 0;
   tuple *ret(*it);
   
   for(; it != values.end(); ++it)
      sum_val += (*it)->get_float(field);
   
   values.pop_front();
   
   ret->set_float(field, sum_val);
   
   return ret;
}

tuple*
agg_configuration::generate(const aggregate_type typ, const field_num field)
{
   switch(typ) {
      case AGG_FIRST: {
         tuple *ret(values.front());
         values.pop_front(); // avoid deleting this tuple
         return ret;
      }
      break;
      case AGG_MAX_INT:
         return generate_max_int(field);
      case AGG_MIN_INT:
         return generate_min_int(field);
      case AGG_SUM_INT:
         return generate_sum_int(field);
      case AGG_SUM_FLOAT:
         return generate_sum_float(field);
   }

#if 0
   AGG_SUM_INT = 4,
   AGG_MAX_FLOAT = 5,
   AGG_MIN_FLOAT = 6,
   AGG_SUM_FLOAT = 7,
   AGG_SET_UNION_INT = 8,
   AGG_SET_UNION_FLOAT = 9,
   AGG_SUM_LIST_INT = 10,
   AGG_SUM_LIST_FLOAT = 11
#endif

   return NULL;
}

agg_configuration::~agg_configuration(void)
{
   for(tuple_list::iterator it(values.begin());
      it != values.end();
      ++it)
   {
      delete *it;
   }
}

void
agg_configuration::print(ostream& cout) const
{
   cout << "\tcount: " << count << endl;
   for(tuple_list::const_iterator it(values.begin());
      it != values.end();
      ++it)
   {
      cout << "\t\t" << **it << endl;
   }
}

ostream&
operator<<(ostream& cout, const agg_configuration& conf)
{
   conf.print(cout);
   return cout;
}

void
tuple_aggregate::add_to_set(vm::tuple *tpl, const ref_count many)
{
   const field_num field(pred->get_aggregate_field());
   
   for(agg_conf_list::iterator it(values.begin());
      it != values.end();
      ++it)
   {
      agg_configuration *conf(*it);
      if(conf->test(tpl, field)) {
         conf->add_to_set(tpl, many);
         return;
      }
   }
   
   // new configuration
   agg_configuration *conf(new agg_configuration());
   conf->add_to_set(tpl, many);
   values.push_back(conf);
}

tuple_aggregate::tuple_list
tuple_aggregate::generate(void)
{
   const aggregate_type typ(pred->get_aggregate_type());
   const field_num field(pred->get_aggregate_field());
   tuple_list ls;
   
   for(agg_conf_list::const_iterator it(values.begin());
      it != values.end();
      ++it)
   {
      ls.push_back((*it)->generate(typ, field));
   }
   
   return ls;
}

tuple_aggregate::~tuple_aggregate(void)
{
   for(agg_conf_list::iterator it(values.begin());
      it != values.end();
      ++it)
   {
      delete *it;
   }
}

void
tuple_aggregate::print(ostream& cout) const
{
   cout << "agg of " << *pred << ":" << endl;
   
   for(agg_conf_list::const_iterator it(values.begin());
      it != values.end();
      ++it)
   {
      cout << **it << endl;
   }
}

ostream&
operator<<(ostream& cout, const tuple_aggregate& agg)
{
   agg.print(cout);
   return cout;
}

}