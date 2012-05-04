
#include <iostream>
#include <assert.h>

#include "db/node.hpp"
#include "vm/state.hpp"
#include "db/neighbor_tuple_aggregate.hpp"

using namespace db;
using namespace std;
using namespace vm;

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
node::add_tuple(vm::tuple *tpl, ref_count many)
{
   const predicate* pred(tpl->get_predicate());
   tuple_trie *tr(get_storage(pred));
   
   if(pred->is_route_pred()) {
      const predicate_id pred_id(pred->get_id());
      edge_map::iterator it(edge_info.find(pred_id));
      
      assert(it != edge_info.end());
      
      edge_set& s(it->second);
      
      s.insert(tpl->get_node(0));
   }
   
   return tr->insert_tuple(tpl, many);
}

node::delete_info
node::delete_tuple(vm::tuple *tuple, ref_count many)
{
   const predicate *pred(tuple->get_predicate());
   tuple_trie *tr(get_storage(pred));
   
   return tr->delete_tuple(tuple, many);
}

agg_configuration*
node::add_agg_tuple(vm::tuple *tuple, const ref_count many)
{
   const predicate *pred(tuple->get_predicate());
   predicate_id pred_id(pred->get_id());
   aggregate_map::iterator it(aggs.find(pred_id));
   tuple_aggregate *agg;
   
   if(it == aggs.end()) {
      // add new
      
      if(aggregate_safeness_uses_neighborhood(pred->get_agg_safeness())) {
         const predicate *remote_pred(pred->get_remote_pred());
         edge_set edges(get_edge_set(remote_pred->get_id()));
         if(pred->get_agg_safeness() == AGG_NEIGHBORHOOD_AND_SELF) {
            edges.insert(id);
         }
         agg = new neighbor_tuple_aggregate(pred, edges);
      } else
         agg = new tuple_aggregate(pred);
            
      aggs[pred_id] = agg;
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

void
node::match_predicate(const predicate_id id, tuple_vector& vec) const
{
   simple_tuple_map::const_iterator it(tuples.find(id));
   
   if(it == tuples.end())
      return;
   
   const tuple_trie *tr(it->second);
   
   tr->match_predicate(vec);
}

void
node::match_predicate(const predicate_id id, const match& m, tuple_vector& vec) const
{
   simple_tuple_map::const_iterator it(tuples.find(id));
   
   if(it == tuples.end())
      return;
   
   const tuple_trie *tr(it->second);
   
   tr->match_predicate(m, vec);
}

void
node::delete_all(const predicate*)
{
   assert(false);
}

void
node::delete_by_leaf(const predicate *pred, tuple_trie_leaf *leaf)
{
   tuple_trie *tr(get_storage(pred));

   tr->delete_by_leaf(leaf);
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

node::node(const node_id _id, const node_id _trans):
   id(_id), translation(_trans)
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

void
node::print(ostream& cout) const
{
   cout << "--> node " << get_translated_id() << "/" << get_id()
        << " (" << this << ") <--" << endl;
   
   for(simple_tuple_map::const_iterator it(tuples.begin());
      it != tuples.end();
      ++it)
   {
      it->second->print(cout);
   }
}

void
node::init(void)
{
   for(size_t i(0); i < state::PROGRAM->num_route_predicates(); ++i) {
      const predicate *pred(state::PROGRAM->get_route_predicate(i));
      
      edge_info[pred->get_id()] = edge_set();
   }
}

ostream&
operator<<(ostream& cout, const node& node)
{
   node.print(cout);
   return cout;
}

}
