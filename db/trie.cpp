
#include <string>

#include "utils/utils.hpp"
#include "db/trie.hpp"

using namespace vm;
using namespace std;

namespace db
{
   
simple_tuple*
trie::look_for_simple_tuple(const simple_tuple_list& list, vm::tuple *tpl)
{
   for(simple_tuple_list::const_iterator it(list.begin());
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
trie::insert_tuple(vm::tuple *tpl, const ref_count many)
{
   simple_tuple *found(look_for_simple_tuple(list, tpl));
   
   if(found != NULL) {
      found->inc_count(many);
      return false;
   } else {
      //cout << "add tuple " << *tuple << endl;
      list.push_back(new simple_tuple(tpl, many));
      return true;
   }
}

void
trie::commit_delete(simple_tuple_list::iterator it)
{
   simple_tuple* stuple(*it);
   vm::tuple *tuple(stuple->get_tuple());
   
   list.erase(it);
   
   delete tuple;
   delete stuple;
}

trie::delete_info
trie::delete_tuple(vm::tuple *tpl, const ref_count many)
{  
   for(simple_tuple_list::iterator it(list.begin());
      it != list.end();
      ++it)
   {
      simple_tuple *stuple(*it);
      
      if(*(stuple->get_tuple()) == *tpl) {
         simple_tuple *target((simple_tuple*)stuple);
         
         target->dec_count(many);
         
         if(target->get_count() == 0)
            return delete_info(this, true, it);
         else
            return delete_info(false);
      }
   }
   
   return delete_info(false); // not reached
}

tuple_vector*
trie::match_predicate(void) const
{
   tuple_vector *ret(new tuple_vector());
   
   for(simple_tuple_list::const_iterator it(list.begin());
      it != list.end();
      it++)
   {
      ret->push_back((*it)->get_tuple());
   }
   
   return ret;
}

void
trie::print(ostream& cout) const
{
   for(simple_tuple_list::const_iterator it(list.begin());
      it != list.end();
      ++it)
   {
      simple_tuple *stuple(*it);
      
      if(it == list.begin())
         cout << " " << *(stuple->get_tuple()->get_predicate()) << ":" << endl;
      
      cout << "\t" << *stuple << endl;
   }
}

void
trie::dump(ostream& cout) const
{
   std::list<string> ls;
   
   for(simple_tuple_list::const_iterator it(list.begin());
      it != list.end();
      ++it)
   {
      simple_tuple *stuple(*it);
      ls.push_back(utils::to_string(*stuple));
   }
   
   ls.sort();
   
   for(std::list<string>::const_iterator it(ls.begin());
      it != ls.end();
      ++it)
   {
      cout << *it << endl;
   }
}

trie::~trie(void)
{
   if(!USE_ALLOCATOR) {
      for(simple_tuple_list::iterator it(list.begin()); it != list.end(); ++it) {
         simple_tuple *stpl(*it);
         delete stpl->get_tuple();
         delete stpl;
      }
   }
}
   
}