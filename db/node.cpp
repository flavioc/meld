
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
node::get_storage(const predicate* pred)
{
   simple_tuple_map::iterator it(tuples.find(pred->get_id()));
   
   if(it == tuples.end()) {
      //cout << "New trie for " << *pred << endl;
      tuple_trie *tr(new tuple_trie(pred));
      tuples[pred->get_id()] = tr;
      return tr;
   } else
      return it->second;
}

bool
node::add_tuple(vm::tuple *tpl, const derivation_count many, const depth_t depth)
{
   const predicate* pred(tpl->get_predicate());
   tuple_trie *tr(get_storage(pred));
   
   bool ret = tr->insert_tuple(tpl, many, depth);
   return ret;
}

node::delete_info
node::delete_tuple(vm::tuple *tuple, const derivation_count many, const depth_t depth)
{
   const predicate *pred(tuple->get_predicate());
   tuple_trie *tr(get_storage(pred));
   
   return tr->delete_tuple(tuple, many, depth);
}

agg_configuration*
node::add_agg_tuple(vm::tuple *tuple, const derivation_count many, const depth_t depth)
{
   const predicate *pred(tuple->get_predicate());
   predicate_id pred_id(pred->get_id());
   aggregate_map::iterator it(aggs.find(pred_id));
   tuple_aggregate *agg;
   
   if(it == aggs.end()) {
      agg = new tuple_aggregate(pred);
      aggs[pred_id] = agg;
   } else
      agg = it->second;

   return agg->add_to_set(tuple, many, depth);
}

agg_configuration*
node::remove_agg_tuple(vm::tuple *tuple, const derivation_count many, const depth_t depth)
{
   return add_agg_tuple(tuple, -many, depth);
}

simple_tuple_list
node::end_iteration(void)
{
   // generate possible aggregates
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

tuple_trie::tuple_search_iterator
node::match_predicate(const predicate_id id) const
{
   simple_tuple_map::const_iterator it(tuples.find(id));
   
   if(it == tuples.end())
      return tuple_trie::tuple_search_iterator();
   
   const tuple_trie *tr(it->second);
   
   return tr->match_predicate();
}

tuple_trie::tuple_search_iterator
node::match_predicate(const predicate_id id, const match* m) const
{
   simple_tuple_map::const_iterator it(tuples.find(id));
   
   if(it == tuples.end())
      return tuple_trie::tuple_search_iterator();
   
   const tuple_trie *tr(it->second);
   
   return tr->match_predicate(m);
}

void
node::delete_all(const predicate*)
{
   assert(false);
}

void
node::delete_by_leaf(const predicate *pred, tuple_trie_leaf *leaf, const depth_t depth)
{
   tuple_trie *tr(get_storage(pred));

   tr->delete_by_leaf(leaf, depth);
}

void
node::assert_tries(void)
{
   for(simple_tuple_map::iterator it(tuples.begin()), end(tuples.end()); it != end; ++it) {
      tuple_trie *tr(it->second);
      tr->assert_used();
   }
}

void
node::delete_by_index(const predicate *pred, const match& m)
{
   tuple_trie *tr(get_storage(pred));
   
   tr->delete_by_index(m);
   
   aggregate_map::iterator it(aggs.find(pred->get_id()));
   
   if(it != aggs.end()) {
      tuple_aggregate *agg(it->second);
      agg->delete_by_index(m);
   }
}

size_t
node::count_total(const predicate_id id) const
{
   simple_tuple_map::const_iterator it(tuples.find(id));
   
   if(it == tuples.end())
      return 0;

   const tuple_trie *tr(it->second);
   
   return tr->size();
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

node::node(const node_id _id, const node_id _trans, vm::all *_all):
   all(_all), id(_id), translation(_trans), owner(NULL), matcher(_all->PROGRAM)
{
}

node::~node(void)
{
   for(simple_tuple_map::iterator it(tuples.begin()), end(tuples.end()); it != end; it++)
      delete it->second;
   for(aggregate_map::iterator it(aggs.begin()), end(aggs.end()); it != end; it++)
      delete it->second;
}

void
node::dump(ostream& cout) const
{
   cout << get_id() << endl;
   
   for(simple_tuple_map::const_iterator it(tuples.begin());
      it != tuples.end();
      ++it)
   {
      it->second->dump(cout);
   }
}

typedef pair<string, tuple_trie*> str_trie;

static inline bool
trie_comparer(const str_trie& p1, const str_trie& p2)
{
	return p1.first < p2.first;
}

void
node::print(ostream& cout) const
{
	typedef list<str_trie> list_str_trie;
	list_str_trie ordered_tries;

	// order tries by predicate name
   for(simple_tuple_map::const_iterator it(tuples.begin());
      it != tuples.end();
      ++it)
   {
		tuple_trie *tr(it->second);
		predicate_id id(it->first);
      const predicate *pred(all->PROGRAM->get_predicate(id));

		ordered_tries.push_back(str_trie(pred->get_name(), tr));
	}

	ordered_tries.sort(trie_comparer);

   cout << "--> node " << get_translated_id() << "/(id " << get_id()
        << ") (" << this << ") <--" << endl;
   
	for(list_str_trie::const_iterator it(ordered_tries.begin());
			it != ordered_tries.end();
			++it)
	{
		tuple_trie *tr(it->second);
		
		if(!tr->empty())
			tr->print(cout);
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

void
node::init(void)
{
}

ostream&
operator<<(ostream& cout, const node& node)
{
   node.print(cout);
   return cout;
}

}
