
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

tuple_trie*
node::get_storage(predicate* pred)
{
   auto it(tuples.find(pred->get_id()));
   
   if(it == tuples.end()) {
      //cout << "New trie for " << *pred << endl;
      tuple_trie *tr(new tuple_trie());
      tuples[pred->get_id()] = tr;
      return tr;
   } else
      return it->second;
}

bool
node::add_tuple(vm::tuple *tpl, vm::predicate *pred, const derivation_count many, const depth_t depth)
{
   tuple_trie *tr(get_storage(pred));
   
   bool ret = tr->insert_tuple(tpl, pred, many, depth);
   return ret;
}

node::delete_info
node::delete_tuple(vm::tuple *tuple, vm::predicate *pred, const derivation_count many, const depth_t depth)
{
   tuple_trie *tr(get_storage(pred));
   
   return tr->delete_tuple(tuple, pred, many, depth);
}

agg_configuration*
node::add_agg_tuple(vm::tuple *tuple, predicate *pred, const derivation_count many, const depth_t depth)
{
   predicate_id pred_id(pred->get_id());
   aggregate_map::iterator it(aggs.find(pred_id));
   tuple_aggregate *agg;
   
   if(it == aggs.end()) {
      agg = new tuple_aggregate(pred);
      aggs[pred_id] = agg;
   } else
      agg = it->second;

   return agg->add_to_set(tuple, pred, many, depth);
}

agg_configuration*
node::remove_agg_tuple(vm::tuple *tuple, vm::predicate *pred, const derivation_count many, const depth_t depth)
{
   return add_agg_tuple(tuple, pred, -many, depth);
}

full_tuple_list
node::end_iteration(void)
{
   // generate possible aggregates
   full_tuple_list ret;
   
   for(aggregate_map::iterator it(aggs.begin());
      it != aggs.end();
      ++it)
   {
      tuple_aggregate *agg(it->second);
      
      full_tuple_list ls(agg->generate());
      for(auto jt(ls.begin()), end(ls.end()); jt != end; ++jt)
         (*jt)->set_as_aggregate();
      
      ret.splice_back(ls);
   }
   
   return ret;
}

tuple_trie::tuple_search_iterator
node::match_predicate(const predicate_id id) const
{
   pers_tuple_map::const_iterator it(tuples.find(id));
   
   if(it == tuples.end())
      return tuple_trie::tuple_search_iterator();
   
   const tuple_trie *tr(it->second);
   
   return tr->match_predicate();
}

tuple_trie::tuple_search_iterator
node::match_predicate(const predicate_id id, const match* m) const
{
   pers_tuple_map::const_iterator it(tuples.find(id));
   
   if(it == tuples.end())
      return tuple_trie::tuple_search_iterator();
   
   const tuple_trie *tr(it->second);
   
   if(m)
      return tr->match_predicate(m);
   else
      return tr->match_predicate();
}

void
node::delete_all(const predicate*)
{
   assert(false);
}

void
node::delete_by_leaf(predicate *pred, tuple_trie_leaf *leaf, const depth_t depth)
{
   tuple_trie *tr(get_storage(pred));

   tr->delete_by_leaf(leaf, pred, depth);
}

void
node::assert_tries(void)
{
   for(auto it(tuples.begin()), end(tuples.end()); it != end; ++it) {
      tuple_trie *tr(it->second);
      tr->assert_used();
   }
}

void
node::delete_by_index(predicate *pred, const match& m)
{
   tuple_trie *tr(get_storage(pred));
   
   tr->delete_by_index(pred, m);
   
   aggregate_map::iterator it(aggs.find(pred->get_id()));
   
   if(it != aggs.end()) {
      tuple_aggregate *agg(it->second);
      agg->delete_by_index(m);
   }
}

size_t
node::count_total(const predicate_id id) const
{
   predicate *pred(theProgram->get_predicate(id));
   if(pred->is_persistent_pred()) {
      auto it(tuples.find(id));
   
      if(it == tuples.end())
         return 0;

      const tuple_trie *tr(it->second);
   
      return tr->size();
   } else {
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
   for(aggregate_map::const_iterator it(aggs.begin());
      it != aggs.end();
      ++it)
   {
      assert(it->second->no_changes());
   }
}

node::node(const node_id _id, const node_id _trans):
   INIT_DOUBLE_QUEUE_NODE(),
   id(_id), translation(_trans), owner(NULL), linear(),
   store(), unprocessed_facts(false)
#ifdef DYNAMIC_INDEXING
   , rounds(0), indexing_epoch(0)
#endif
{
}

void
node::wipeout(void)
{
   for(auto it(tuples.begin()), end(tuples.end()); it != end; it++) {
      tuple_trie *tr(it->second);
      predicate *pred(theProgram->get_predicate(it->first));
      tr->wipeout(pred);
      delete it->second;
   }
   for(aggregate_map::iterator it(aggs.begin()), end(aggs.end()); it != end; it++)
      delete it->second;
}

void
node::dump(ostream& cout) const
{
   cout << get_id() << endl;
   
   for(size_t i(0); i < theProgram->num_predicates(); ++i) {
      predicate *pred(theProgram->get_sorted_predicate(i));

      vector<string> vec;
      if(pred->is_persistent_pred()) {
         auto it(tuples.find(pred->get_id()));
         tuple_trie *tr = NULL;
         if(it != tuples.end())
            tr = it->second;
         if(tr && !tr->empty())
            vec = tr->get_print_strings(pred);
      } else {
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
      if(pred->is_persistent_pred()) {
         auto it(tuples.find(pred->get_id()));
         tuple_trie *tr = NULL;
         if(it != tuples.end())
            tr = it->second;
         if(tr && !tr->empty())
            vec = tr->get_print_strings(pred);
      } else {
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
