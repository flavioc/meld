
#include <iostream>
#include <assert.h>

#include "db/node.hpp"

using namespace db;
using namespace std;
using namespace vm;

namespace db
{

node::simple_tuple_list&
node::get_storage(const predicate_id& id)
{
   simple_tuple_map::iterator it(tuples.find(id));
   
   if(it == tuples.end()) {
      tuples[id] = simple_tuple_list();
      return tuples[id];
   }
   
   return it->second;
}

static inline simple_tuple*
look_for_simple_tuple(const node::simple_tuple_list& list, vm::tuple *tpl)
{
   for(node::simple_tuple_list::const_iterator it(list.begin());
      it != list.end();
      ++it)
   {
      simple_tuple *stuple(*it);
      
      if(*(stuple->get_tuple()) == *tpl)
         return (simple_tuple*)stuple;
   }
   
   return NULL;
}

bool
node::add_tuple(vm::tuple *tpl, ref_count many)
{  
   predicate_id id(tpl->get_predicate()->get_id());
   simple_tuple_list& list(get_storage(id));
   
   simple_tuple *found(look_for_simple_tuple(list, tpl));
   
   if(found != NULL) {
      cout << "FOUND tuple " << *tpl << endl;
      found->inc_count(many);
      return false;
   } else {
      //cout << "add tuple " << *tuple << endl;
      list.push_back(new simple_tuple(tpl, many));
      return true;
   }
}

node::delete_info
node::delete_tuple(vm::tuple *tuple, ref_count many)
{
   predicate_id id(tuple->get_predicate()->get_id());
   simple_tuple_list& list(get_storage(id));
   delete_info ret;
   
   ret.to_delete = false;
   
   for(node::simple_tuple_list::iterator it(list.begin());
      it != list.end();
      ++it)
   {
      simple_tuple *stuple(*it);
      
      if(*(stuple->get_tuple()) == *tuple) {
         simple_tuple *target((simple_tuple*)stuple);
         
         target->dec_count(many);
         
         if(target->get_count() == 0) {
            ret.to_delete = true;
            ret.it = it;
            ret.list = &list;
         }
         
         return ret;
      }
   }
   return ret;
}

void
node::commit_delete(const delete_info& info)
{
   simple_tuple* stuple(*(info.it));
   vm::tuple *tuple(stuple->get_tuple());
   
   info.list->erase(info.it);
   
   delete tuple;
   delete stuple;
}

void
node::add_agg_tuple(vm::tuple *tuple, const ref_count many)
{
   const predicate *pred(tuple->get_predicate());
   predicate_id id(pred->get_id());
   aggregate_map::iterator it(aggs.find(id));
   tuple_aggregate *agg;
   
   if(it == aggs.end())
      // add new
      agg = aggs[id] = new tuple_aggregate(pred);
   else
      agg = it->second;

   agg->add_to_set(tuple, many);
   cout << *agg << endl;
}

list<tuple*>
node::generate_aggs(void)
{
   list<tuple*> ret;
   
   for(aggregate_map::iterator it(aggs.begin());
      it != aggs.end();
      ++it)
   {
      tuple_aggregate *agg(it->second);
      
      list<tuple*> ls(agg->generate());
      
      ret.insert(ret.end(), ls.begin(), ls.end());
      
      delete agg;
   }
   
   aggs.clear();
   
   return ret;
}

node::tuple_vector*
node::match_predicate(const predicate_id id) const
{
   tuple_vector *ret(new tuple_vector());
   
   simple_tuple_map::const_iterator it(tuples.find(id));
   
   if(it == tuples.end())
      return ret;
   
   const simple_tuple_list& list(it->second);
   
   for(simple_tuple_list::const_iterator it(list.begin());
      it != list.end();
      it++)
   {
      ret->push_back((*it)->get_tuple());
   }
   
   return ret;
}

node::~node(void)
{
   for(simple_tuple_map::iterator it(tuples.begin()); it != tuples.end(); ++it) {
      simple_tuple_list& list(it->second);
      
      for(simple_tuple_list::iterator it2(list.begin()); it2 != list.end(); ++it2) {
         simple_tuple *stpl(*it2);
         delete stpl->get_tuple();
         delete stpl;
      }
   }
}

void
node::print(ostream& cout) const
{
   cout << "--> node " << get_translated_id() << "/" << get_id()
        << " (" << (addr_val)real_id() << ") <--" << endl;
   
   for(simple_tuple_map::const_iterator it(tuples.begin());
      it != tuples.end();
      ++it)
   {
      const simple_tuple_list& list(it->second);
      
      for(simple_tuple_list::const_iterator it2(list.begin());
         it2 != list.end();
         ++it2)
      {
         simple_tuple *stuple(*it2);
         
         if(it2 == list.begin()) {
            cout << " " << *(stuple->get_tuple()->get_predicate()) << ":" << endl;
         }
         
         cout << "\t" << *stuple << endl;
      }
   }
}

ostream&
operator<<(ostream& cout, const node& node)
{
   node.print(cout);
   return cout;
}

}