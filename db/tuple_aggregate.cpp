
#include "db/tuple_aggregate.hpp"

using namespace vm;
using namespace std;

namespace db
{
   
agg_configuration*
tuple_aggregate::add_to_set(vm::tuple *tpl, const ref_count many)
{
   /*
   cout << "Adding " << *tpl << endl;
   cout << "Before" << endl;
   print(cout);
   */
   
   agg_trie_leaf *leaf(vals.find_configuration(tpl));
   agg_configuration *conf;
   
   if(leaf->get_conf() == NULL) {
      conf = new agg_configuration(pred);
      leaf->set_conf(conf);
   } else {
      conf = leaf->get_conf();
   }
   
   /*
   cout << "After\n";
   print(cout);
   */
   conf->add_to_set(tpl, many);
   
   return conf;
}

simple_tuple_list
tuple_aggregate::generate(void)
{
   const aggregate_type typ(pred->get_aggregate_type());
   const field_num field(pred->get_aggregate_field());
   simple_tuple_list ls;
   
   for(agg_trie::iterator it(vals.begin());
      it != vals.end(); )
   {
      agg_configuration *conf(*it);
      
      assert(conf != NULL);
      
      if(conf->has_changed())
         conf->generate(typ, field, ls);
      
      assert(!conf->has_changed());
      
      if(conf->is_empty())
         it = vals.erase(it);
      else
         it++;
   }
   
   return ls;
}

const bool
tuple_aggregate::no_changes(void) const
{
   for(agg_trie::const_iterator it(vals.begin());
      it != vals.end();
      ++it)
   {
      agg_configuration *conf(*it);
      
      if(conf->has_changed())
         return false;
   }
   
   return true;
}

void
tuple_aggregate::delete_by_first_int_arg(const int_val val)
{
   /*
   cout << "Before " << val << endl;
   print(cout);*/
   
   vals.delete_by_first_int_arg(val);
   /*cout << "After" << endl;
   print(cout);*/
}

tuple_aggregate::~tuple_aggregate(void)
{
}

void
tuple_aggregate::print(ostream& cout) const
{
   cout << "agg of " << *pred << ":" << endl;
   
   for(agg_trie::const_iterator it(vals.begin());
      it != vals.end();
      it++)
   {
      agg_configuration *conf(*it);
      assert(conf != NULL);
      cout << *conf << endl;
   }
}

ostream&
operator<<(ostream& cout, const tuple_aggregate& agg)
{
   agg.print(cout);
   return cout;
}

}