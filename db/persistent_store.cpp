
#include "db/persistent_store.hpp"
#include "vm/all.hpp"

using namespace db;
using namespace vm;
using namespace std;

tuple_trie*
persistent_store::get_storage(predicate* pred)
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
persistent_store::add_tuple(vm::tuple *tpl, vm::predicate *pred, const depth_t depth)
{
   tuple_trie *tr(get_storage(pred));
   
   bool ret = tr->insert_tuple(tpl, pred, depth);
   return ret;
}

persistent_store::delete_info
persistent_store::delete_tuple(vm::tuple *tuple, vm::predicate *pred, const depth_t depth)
{
   tuple_trie *tr(get_storage(pred));
   
   return tr->delete_tuple(tuple, pred, depth);
}

agg_configuration*
persistent_store::add_agg_tuple(vm::tuple *tuple, predicate *pred,
      const depth_t depth, candidate_gc_nodes& gc_nodes, const derivation_direction dir)
{
   predicate_id pred_id(pred->get_id());
   aggregate_map::iterator it(aggs.find(pred_id));
   tuple_aggregate *agg;
   
   if(it == aggs.end()) {
      agg = new tuple_aggregate(pred);
      aggs[pred_id] = agg;
   } else
      agg = it->second;

   return agg->add_to_set(tuple, pred, dir, depth, gc_nodes);
}

agg_configuration*
persistent_store::remove_agg_tuple(vm::tuple *tuple, vm::predicate *pred,
      const depth_t depth, candidate_gc_nodes& gc_nodes)
{
   return add_agg_tuple(tuple, pred, depth, gc_nodes, NEGATIVE_DERIVATION);
}

full_tuple_list
persistent_store::end_iteration()
{
   // generate possible aggregates
   full_tuple_list ret;
   
   for(aggregate_map::iterator it(aggs.begin());
      it != aggs.end();
      ++it)
   {
      tuple_aggregate *agg(it->second);
      
      full_tuple_list ls(agg->generate());

      for(auto jt(ls.begin()), end(ls.end()); jt != end; ++jt) {
         (*jt)->set_as_aggregate();
         ret.push_back(*jt);
      }
   }
   
   return ret;
}

tuple_trie::tuple_search_iterator
persistent_store::match_predicate(const predicate_id id) const
{
   pers_tuple_map::const_iterator it(tuples.find(id));
   
   if(it == tuples.end())
      return tuple_trie::tuple_search_iterator();
   
   const tuple_trie *tr(it->second);
   
   return tr->match_predicate();
}

tuple_trie::tuple_search_iterator
persistent_store::match_predicate(const predicate_id id, const match* m) const
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
persistent_store::delete_all(const predicate*)
{
   assert(false);
}

void
persistent_store::delete_by_leaf(predicate *pred, tuple_trie_leaf *leaf, const depth_t depth, candidate_gc_nodes& gc_nodes)
{
   tuple_trie *tr(get_storage(pred));

   tr->delete_by_leaf(leaf, pred, depth, gc_nodes);
}

void
persistent_store::assert_tries(void)
{
   for(auto it(tuples.begin()), end(tuples.end()); it != end; ++it) {
      tuple_trie *tr(it->second);
      tr->assert_used();
   }
}

void
persistent_store::delete_by_index(predicate *pred, const match& m, candidate_gc_nodes& gc_nodes)
{
   tuple_trie *tr(get_storage(pred));
   
   tr->delete_by_index(pred, m, gc_nodes);
   
   aggregate_map::iterator it(aggs.find(pred->get_id()));
   
   if(it != aggs.end()) {
      tuple_aggregate *agg(it->second);
      agg->delete_by_index(m, gc_nodes);
   }
}

size_t
persistent_store::count_total(const predicate_id id) const
{
   auto it(tuples.find(id));

   if(it == tuples.end())
      return 0;

   const tuple_trie *tr(it->second);

   return tr->size();
}

void
persistent_store::wipeout(candidate_gc_nodes& gc_nodes)
{
   for(auto it(tuples.begin()), end(tuples.end()); it != end; it++) {
      tuple_trie *tr(it->second);
      predicate *pred(theProgram->get_predicate(it->first));
      tr->wipeout(pred, gc_nodes);
      delete it->second;
   }
   for(aggregate_map::iterator it(aggs.begin()), end(aggs.end()); it != end; it++) {
      tuple_aggregate *agg(it->second);
      agg->wipeout(gc_nodes);
      delete agg;
   }
}

vector<string>
persistent_store::dump(const predicate *pred) const
{
   auto it(tuples.find(pred->get_id()));
   tuple_trie *tr = nullptr;
   if(it != tuples.end()) {
      tr = it->second;
      if(tr && !tr->empty())
         return tr->get_print_strings(pred);
   }
   return vector<string>();
}

vector<string>
persistent_store::print(const predicate *pred) const
{
   auto it(tuples.find(pred->get_id()));
   tuple_trie *tr = nullptr;
   if(it != tuples.end()) {
      tr = it->second;
      if(tr && !tr->empty())
         return tr->get_print_strings(pred);
   }
   return vector<string>();
}
