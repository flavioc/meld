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
   assert(many != 0);
   assert(many > 0 || (many < 0 && !values.empty()));
   
   changed = true; // this is important
   
   for(simple_tuple_list::iterator it(values.begin());
      it != values.end();
      ++it)
   {
      simple_tuple *sother(*it);
      vm::tuple *other(sother->get_tuple());
      
      if(*other == *tpl) {
         sother->add_count(many);
         
         if(sother->reached_zero()) {
            values.erase(it);
            delete sother->get_tuple();
            delete sother;
         }
         
         delete tpl;
         return;
      }
   }
   
   // new one
   values.push_back(new simple_tuple(tpl, many));
}

const bool
agg_configuration::test(vm::tuple *tpl, const field_num agg_field) const
{
   if(values.empty())
      return false;
      
   assert(!values.empty());
   
   simple_tuple_list::const_iterator it(values.begin());
   simple_tuple *sother(*it);
   vm::tuple *other(sother->get_tuple());
   
   for(field_num i(0); i < agg_field; ++i)
      if(!tpl->field_equal(*other, i))
         return false;
   
   return true;
}

vm::tuple*
agg_configuration::generate_max_int(const field_num field) const
{
   simple_tuple_list::const_iterator it(values.begin());
   vm::tuple *max_tpl((*it)->get_tuple());
   int_val max_val(max_tpl->get_int(field));
   
   ++it;
   for(; it != values.end(); ++it)
   {
      simple_tuple *sother(*it);
      vm::tuple *other(sother->get_tuple());
      
      if(max_val < other->get_int(field)) {
         max_val = other->get_int(field);
         max_tpl = other;
      }
   }
   
   return max_tpl->copy();
}

vm::tuple*
agg_configuration::generate_min_int(const field_num field) const
{
   simple_tuple_list::const_iterator it(values.begin());
   vm::tuple *min_tpl((*it)->get_tuple());
   int_val min_val(min_tpl->get_int(field));
   
   ++it;
   for(; it != values.end(); ++it) {
      simple_tuple *sother(*it);
      vm::tuple *other(sother->get_tuple());
      
      if(min_val > other->get_int(field)) {
         min_val = other->get_int(field);
         min_tpl = other;
      }
   }
   
   return min_tpl->copy();
}

vm::tuple*
agg_configuration::generate_sum_int(const field_num field) const
{
   simple_tuple_list::const_iterator it(values.begin());
   int_val sum_val = 0;
   vm::tuple *ret((*it)->get_tuple()->copy());
   
   for(; it != values.end(); ++it) {
      simple_tuple *s(*it);
      
      for(ref_count i(0); i < s->get_count(); ++i)
         sum_val += s->get_tuple()->get_int(field);
   }
   
   ret->set_int(field, sum_val);
   return ret;
}

vm::tuple*
agg_configuration::generate_sum_float(const field_num field) const
{
   simple_tuple_list::const_iterator it(values.begin());
   float_val sum_val = 0;
   vm::tuple *ret((*it)->get_tuple()->copy());
   
   for(; it != values.end(); ++it) {
      simple_tuple *s(*it);
      
      for(ref_count i(0); i < s->get_count(); ++i)
         sum_val += (*it)->get_tuple()->get_float(field);
   }
   
   ret->set_float(field, sum_val);
   
   return ret;
}

vm::tuple*
agg_configuration::generate_first(void) const
{
   simple_tuple *s(values.front());
   
   return s->get_tuple()->copy();
}

vm::tuple*
agg_configuration::generate_max_float(const field_num field) const
{
   simple_tuple_list::const_iterator it(values.begin());
   vm::tuple *max_tpl((*it)->get_tuple());
   float_val max_val(max_tpl->get_float(field));
   
   ++it;
   for(; it != values.end(); ++it) {
      simple_tuple *sother(*it);
      vm::tuple *other(sother->get_tuple());
      
      if(max_val < other->get_float(field)) {
         max_val = other->get_float(field);
         max_tpl = other;
      }
   }
   
   return max_tpl->copy();
}

vm::tuple*
agg_configuration::generate_min_float(const field_num field) const
{
   simple_tuple_list::const_iterator it(values.begin());
   vm::tuple *min_tpl((*it)->get_tuple());
   float_val min_val(min_tpl->get_float(field));
   
   ++it;
   for(; it != values.end(); ++it) {
      simple_tuple *sother(*it);
      vm::tuple *other(sother->get_tuple());
      
      if(min_val > other->get_float(field)) {
         min_val = other->get_float(field);
         min_tpl = other;
      }
   }
   
   return min_tpl->copy();
}

vm::tuple*
agg_configuration::do_generate(const aggregate_type typ, const field_num field)
{
   if(values.empty())
      return NULL;
   
   switch(typ) {
      case AGG_FIRST:
         return generate_first();
      case AGG_MAX_INT:
         return generate_max_int(field);
      case AGG_MIN_INT:
         return generate_min_int(field);
      case AGG_SUM_INT:
         return generate_sum_int(field);
      case AGG_SUM_FLOAT:
         return generate_sum_float(field);
      case AGG_MAX_FLOAT:
         return generate_max_float(field);
      case AGG_MIN_FLOAT:
         return generate_min_float(field);
   }
}

void
agg_configuration::generate(const aggregate_type typ, const field_num field,
   simple_tuple_list& cont)
{
   vm::tuple* generated(do_generate(typ, field));
   
   changed = false;
   
   if(corresponds == NULL) {
      // new
      if(generated != NULL)
         cont.push_back(simple_tuple::create_new(generated));
   } else if (generated == NULL) {
      if(corresponds != NULL)
         cont.push_back(simple_tuple::remove_new(corresponds));
   } else {
      if(!(*corresponds == *generated)) {
         cont.push_back(simple_tuple::remove_new(corresponds));
         cont.push_back(simple_tuple::create_new(generated));
      }
   }
   
   corresponds = generated;
}

agg_configuration::~agg_configuration(void)
{
   for(simple_tuple_list::iterator it(values.begin());
      it != values.end();
      ++it)
   {
      delete (*it)->get_tuple();
      delete *it;
   }
}

void
agg_configuration::print(ostream& cout) const
{
   for(simple_tuple_list::const_iterator it(values.begin());
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

simple_tuple_list
tuple_aggregate::generate(void)
{
   const aggregate_type typ(pred->get_aggregate_type());
   const field_num field(pred->get_aggregate_field());
   simple_tuple_list ls;
   
   for(agg_conf_list::iterator it(values.begin());
      it != values.end();
      ++it)
   {
      agg_configuration *conf(*it);
      
      if(conf->has_changed())
         conf->generate(typ, field, ls);
      if(conf->is_empty()) {
         it = values.erase(it);
         delete conf;
      }
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