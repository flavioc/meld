
#include <iostream>
#include <assert.h>

#include "db/node.hpp"

using namespace db;
using namespace std;
using namespace vm;

namespace db
{

node::stuple_list&
node::get_storage(const predicate_id& id)
{
   stuple_map::iterator it(tuples.find(id));
   
   if(it == tuples.end()) {
      tuples[id] = stuple_list();
      return tuples[id];
   }
   
   return it->second;
}

static inline simple_tuple*
look_for_stuple(const node::stuple_list& list, vm::tuple *tpl)
{
   for(node::stuple_list::const_iterator it(list.begin());
      it != list.end();
      ++it)
   {
      stuple *stuple(*it);
      
      if(*(stuple->get_tuple()) == *tpl)
         return (simple_tuple*)stuple;
   }
   
   return NULL;
}

bool
node::add_tuple(vm::tuple *tpl, ref_count many)
{  
   predicate_id id(tpl->get_predicate()->get_id());
   stuple_list& list(get_storage(id));
   
   simple_tuple *found(look_for_stuple(list, tpl));
   
   if(found != NULL) {
      cout << "FOUND tuple " << *tpl << endl;
      cout << *this << endl;
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
   stuple_list& list(get_storage(id));
   delete_info ret;
   
   ret.to_delete = false;
   
   for(node::stuple_list::iterator it(list.begin());
      it != list.end();
      ++it)
   {
      stuple *stuple(*it);
      
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
   stuple* stuple(*(info.it));
   vm::tuple *tuple(stuple->get_tuple());
   
   info.list->erase(info.it);
   
   delete tuple;
   delete stuple;
}

node::tuple_vector*
node::match_predicate(const predicate_id id) const
{
   tuple_vector *ret(new tuple_vector());
   
   stuple_map::const_iterator it(tuples.find(id));
   
   if(it == tuples.end())
      return ret;
   
   const stuple_list& list(it->second);
   
   for(stuple_list::const_iterator it(list.begin());
      it != list.end();
      it++)
   {
      ret->push_back((*it)->get_tuple());
   }
   
   return ret;
}

void
node::print(ostream& cout) const
{
   cout << "--> node " << get_id() << " (" << (addr_val)real_id() << ") <--" << endl;
   
   for(stuple_map::const_iterator it(tuples.begin());
      it != tuples.end();
      ++it)
   {
      const stuple_list& list(it->second);
      
      for(stuple_list::const_iterator it2(list.begin());
         it2 != list.end();
         ++it2)
      {
         stuple *stuple(*it2);
         
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