
#include "db/persistent_store.hpp"
#include "vm/all.hpp"

using namespace db;
using namespace vm;
using namespace std;

persistent_store::persistent_store()
{
#ifndef COMPILED
   tuples = mem::allocator<tuple_trie>().allocate(vm::theProgram->num_persistent_predicates());
   for(size_t i(0); i < vm::theProgram->num_persistent_predicates(); ++i)
      mem::allocator<tuple_trie>().construct(tuples + i);
#endif
}

tuple_trie*
persistent_store::get_storage(const predicate* pred) const
{
#ifdef COMPILED
   return (tuple_trie*)(&tuples[pred->get_persistent_id()]);
#else
   return tuples + pred->get_persistent_id();
#endif
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
   auto it(aggs.find(pred_id));
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
   
   for(auto & elem : aggs)
   {
      tuple_aggregate *agg(elem.second);
      
      full_tuple_list ls(agg->generate());

      for(auto && l : ls) {
         (l)->set_as_aggregate();
         ret.push_back(l);
      }
   }
   
   return ret;
}

void
persistent_store::delete_by_leaf(predicate *pred, tuple_trie_leaf *leaf, const depth_t depth, candidate_gc_nodes& gc_nodes)
{
   tuple_trie *tr(get_storage(pred));

   tr->delete_by_leaf(leaf, pred, depth, gc_nodes);
}

void
persistent_store::delete_by_index(predicate *pred, const match& m, candidate_gc_nodes& gc_nodes)
{
   tuple_trie *tr(get_storage(pred));
   
   tr->delete_by_index(pred, m, gc_nodes);
   
   auto it(aggs.find(pred->get_id()));
   
   if(it != aggs.end()) {
      tuple_aggregate *agg(it->second);
      agg->delete_by_index(m, gc_nodes);
   }
}

size_t
persistent_store::count_total(const predicate* pred) const
{
   tuple_trie *tr(get_storage(pred));

   return tr->size();
}

void
persistent_store::wipeout(candidate_gc_nodes& gc_nodes)
{
   for(auto *pred : vm::theProgram->persistent_predicates) {
      tuple_trie *tr(get_storage(pred));
      tr->wipeout(pred, gc_nodes);
#ifndef COMPILED
      mem::allocator<tuple_trie>().destroy(tr);
#endif
   }
   for(auto & elem : aggs) {
      tuple_aggregate *agg(elem.second);
      agg->wipeout(gc_nodes);
      delete agg;
   }
#ifndef COMPILED
   mem::allocator<tuple_trie>().deallocate(tuples, vm::theProgram->num_persistent_predicates());
#endif
}

vector<string>
persistent_store::dump(const predicate *pred) const
{
   tuple_trie *tr(get_storage(pred));
   if(!tr->empty())
      return tr->get_print_strings(pred);
   return vector<string>();
}

vector<string>
persistent_store::print(const predicate *pred) const
{
   tuple_trie *tr(get_storage(pred));
   if(!tr->empty())
         return tr->get_print_strings(pred);
   return vector<string>();
}
