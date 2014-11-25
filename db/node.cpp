
#include <iostream>
#include <assert.h>

#include "db/node.hpp"
#include "vm/state.hpp"
#include "utils/utils.hpp"

#ifdef USE_UI
#include "ui/macros.hpp"
#endif

using namespace db;
using namespace std;
using namespace vm;
using namespace utils;

namespace db
{

size_t
node::count_total(const predicate_id id) const
{
   predicate *pred(theProgram->get_predicate(id));
   if(pred->is_persistent_pred())
      return pers_store.count_total(id);
   else {
      if(linear.stored_as_hash_table(pred)) {
         const hash_table *table(linear.get_hash_table(pred->get_id()));
         return table->get_total_size();
      } else {
         const intrusive_list<vm::tuple> *ls(linear.get_linked_list(pred->get_id()));
         return ls->get_size();
      }
   }
}

size_t
node::count_total_all(void) const
{
   size_t total(0);
   for(size_t i(0); i < theProgram->num_predicates(); ++i) {
      total += count_total(i);
   }
   return total;
}

void
node::assert_end(void) const
{
}

node::node(const node_id _id, const node_id _trans):
   refs(0),
   id(_id), translation(_trans),
   default_priority_level(no_priority_value()),
   priority_level(theProgram->get_initial_priority())
{
}

void
node::wipeout(candidate_gc_nodes& gc_nodes)
{
   linear.destroy(gc_nodes);
   pers_store.wipeout(gc_nodes);

   mem::allocator<node>().deallocate(this, 1);
}

void
node::dump(ostream& cout) const
{
   cout << get_id() << endl;
   
   for(size_t i(0); i < theProgram->num_predicates(); ++i) {
      predicate *pred(theProgram->get_sorted_predicate(i));

      vector<string> vec;
      if(pred->is_persistent_pred())
         vec = pers_store.dump(pred);
      else {
         if(linear.stored_as_hash_table(pred)) {
            const hash_table *table(linear.get_hash_table(pred->get_id()));
            for(hash_table::iterator it(table->begin()); !it.end(); ++it) {
               const intrusive_list<vm::tuple> *ls(*it);
               if(!ls->empty()) {
                  for(intrusive_list<vm::tuple>::const_iterator it(ls->begin()), end(ls->end());
                        it != end; ++it)
                     vec.push_back((*it)->to_str(pred));
               }
            }
         } else {
            const intrusive_list<vm::tuple> *ls(linear.get_linked_list(pred->get_id()));
            if(ls && !ls->empty()) {
               for(intrusive_list<vm::tuple>::const_iterator it(ls->begin()), end(ls->end());
                     it != end; ++it)
                  vec.push_back(to_string((*it)->to_str(pred)));
            }
         }
      }

      if(!vec.empty()) {
         sort(vec.begin(), vec.end());

         for(size_t i(0); i < vec.size(); ++i)
            cout << vec[i] << endl;
      }
   }
}

void
node::print(ostream& cout) const
{
   cout << "--> node " << get_translated_id() << "/(id " << get_id()
        << ") (" << this;
#ifdef DYNAMIC_INDEXING
   cout << "/" << rounds;
#endif
  cout << ") <--" << endl;
   
   for(size_t i(0); i < theProgram->num_predicates(); ++i) {
      predicate *pred(theProgram->get_sorted_predicate(i));

      vector<string> vec;
      if(pred->is_persistent_pred())
         vec = pers_store.print(pred);
      else {
         if(linear.stored_as_hash_table(pred)) {
            const hash_table *table(linear.get_hash_table(pred->get_id()));
            for(hash_table::iterator it(table->begin()); !it.end(); ++it) {
               const intrusive_list<vm::tuple> *ls(*it);
               if(!ls->empty()) {
                  for(intrusive_list<vm::tuple>::const_iterator it(ls->begin()), end(ls->end());
                        it != end; ++it)
                     vec.push_back((*it)->to_str(pred));
               }
            }
         } else {
            const intrusive_list<vm::tuple> *ls(linear.get_linked_list(pred->get_id()));
            if(ls && !ls->empty()) {
               for(intrusive_list<vm::tuple>::iterator it(ls->begin()), end(ls->end());
                     it != end; ++it)
                  vec.push_back((*it)->to_str(pred));
            }
         }
      }

      if(vec.empty())
         continue;

      cout << " ";
      pred->print_simple(cout);
      cout << endl;

      sort(vec.begin(), vec.end());
      write_strings(vec, cout, 1);
   }
}

#ifdef USE_UI
using namespace json_spirit;

Value
node::dump_json(void) const
{
	Object ret;
	
	for(simple_tuple_map::const_iterator it(tuples.begin());
      it != tuples.end();
      ++it)
	{
		predicate_id pred(it->first);
		tuple_trie *trie(it->second);
		Array tpls;
		
		for(tuple_trie::const_iterator jt(trie->begin()), end(trie->end()); jt != end; jt++)
	   {
			tuple_trie_leaf *leaf(*jt);
			tuple *tpl(leaf->get_underlying_tuple());
			UI_ADD_ELEM(tpls, tpl->dump_json());
		}
		
		UI_ADD_FIELD(ret, to_string((int)pred), tpls);
	}
	
	return ret;
}
#endif

ostream&
operator<<(ostream& cout, const node& node)
{
   node.print(cout);
   return cout;
}

}
