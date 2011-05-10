
#include "db/tuple_aggregate.hpp"

using namespace vm;
using namespace std;

namespace db
{
   
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