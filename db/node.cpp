
#include <iostream>
#include <assert.h>

#include "db/node.hpp"

using namespace db;
using namespace std;
using namespace vm;

namespace db
{

trie&
node::get_storage(const predicate_id& id)
{
   simple_tuple_map::iterator it(tuples.find(id));
   
   if(it == tuples.end()) {
      tuples[id] = trie();
      it = tuples.find(id);
   }
   
   return it->second;
}

bool
node::add_tuple(vm::tuple *tpl, ref_count many)
{
   predicate_id id(tpl->get_predicate()->get_id());
   trie& tr(get_storage(id));
   
   return tr.insert_tuple(tpl, many);
}

node::delete_info
node::delete_tuple(vm::tuple *tuple, ref_count many)
{
   const predicate_id id(tuple->get_predicate_id());
   trie& tr(get_storage(id));
   
   return tr.delete_tuple(tuple, many);
}

agg_configuration*
node::add_agg_tuple(vm::tuple *tuple, const ref_count many)
{
   const predicate *pred(tuple->get_predicate());
   predicate_id id(pred->get_id());
   aggregate_map::iterator it(aggs.find(id));
   tuple_aggregate *agg;
   
   if(it == aggs.end()) {
      // add new
      agg = new tuple_aggregate(pred);
      aggs[id] = agg;
   } else
      agg = it->second;

   return agg->add_to_set(tuple, many);
}

void
node::remove_agg_tuple(vm::tuple *tuple, const ref_count many)
{
   add_agg_tuple(tuple, -many);
}

simple_tuple_list
node::generate_aggs(void)
{
   simple_tuple_list ret;
   
   for(aggregate_map::iterator it(aggs.begin());
      it != aggs.end();
      ++it)
   {
      tuple_aggregate *agg(it->second);
      
      simple_tuple_list ls(agg->generate());
      
      ret.insert(ret.end(), ls.begin(), ls.end());
   }
   
   return ret;
}

tuple_vector*
node::match_predicate(const predicate_id id) const
{
   tuple_vector *ret(new tuple_vector());
   
   simple_tuple_map::const_iterator it(tuples.find(id));
   
   if(it == tuples.end())
      return ret;
   
   const trie& tr(it->second);
   
   return tr.match_predicate();
}

node::~node(void)
{
}

void
node::dump(ostream& cout) const
{
   cout << get_id() << endl;
   
   for(simple_tuple_map::const_iterator it(tuples.begin());
      it != tuples.end();
      ++it)
   {
      it->second.dump(cout);
   }
}

void
node::print(ostream& cout) const
{
   cout << "--> node " << get_translated_id() << "/" << get_id()
        << " (" << this << ") <--" << endl;
   
   for(simple_tuple_map::const_iterator it(tuples.begin());
      it != tuples.end();
      ++it)
   {
      it->second.print(cout);
   }
}

ostream&
operator<<(ostream& cout, const node& node)
{
   node.print(cout);
   return cout;
}

}
