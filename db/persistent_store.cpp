
#include "db/persistent_store.hpp"
#include "vm/all.hpp"

using namespace db;
using namespace vm;
using namespace std;

persistent_store::delete_info
persistent_store::delete_tuple(vm::tuple *tuple, vm::predicate *pred, const depth_t depth)
{
   tuple_trie *tr(get_storage(pred));
   
   return tr->delete_tuple(tuple, pred, depth);
}

#ifndef COMPILED_NO_AGGREGATES
agg_configuration*
persistent_store::add_agg_tuple(vm::tuple *tuple, predicate *pred,
      const depth_t depth, const derivation_direction dir,
      mem::node_allocator *alloc, candidate_gc_nodes& gc_nodes)
{
   predicate_id pred_id(pred->get_persistent_id());
#ifdef COMPILED
   tuple_aggregate *agg(aggs[pred_id]);
   if(!agg)
      agg = aggs[pred_id] = new tuple_aggregate(pred);
#else
   auto it(aggs.find(pred_id));
   tuple_aggregate *agg;
   
   if(it == aggs.end()) {
      agg = new tuple_aggregate(pred);
      aggs[pred_id] = agg;
   } else
      agg = it->second;
#endif

   return agg->add_to_set(tuple, pred, dir, depth, alloc, gc_nodes);
}

agg_configuration*
persistent_store::remove_agg_tuple(vm::tuple *tuple, vm::predicate *pred,
      const depth_t depth, mem::node_allocator *alloc, candidate_gc_nodes& gc_nodes)
{
   return add_agg_tuple(tuple, pred, depth, NEGATIVE_DERIVATION, alloc, gc_nodes);
}
#endif

full_tuple_list
persistent_store::end_iteration(mem::node_allocator *alloc)
{
   full_tuple_list ret;
#ifndef COMPILED_NO_AGGREGATES
   // generate possible aggregates
   
#ifdef COMPILED
   for(size_t i(0); i < COMPILED_NUM_TRIES; ++i) {
      tuple_aggregate *agg(aggs[i]);
      if(aggs) {
         full_tuple_list ls(agg->generate());
         for(auto && l : ls) {
            (l)->set_as_aggregate();
            ret.push_back(l);
         }
      }
   }
#else
   for(auto & elem : aggs)
   {
      tuple_aggregate *agg(elem.second);
      
      full_tuple_list ls(agg->generate(alloc));

      for(auto && l : ls) {
         (l)->set_as_aggregate();
         ret.push_back(l);
      }
   }
#endif
#endif
   
   return ret;
}

void
persistent_store::delete_by_leaf(predicate *pred, tuple_trie_leaf *leaf, const depth_t depth,
      mem::node_allocator *alloc, candidate_gc_nodes& gc_nodes)
{
   tuple_trie *tr(get_storage(pred));

   tr->delete_by_leaf(leaf, pred, depth, alloc, gc_nodes);
}

void
persistent_store::delete_by_index(predicate *pred, const match& m,
      mem::node_allocator *alloc, candidate_gc_nodes& gc_nodes)
{
   tuple_trie *tr(get_storage(pred));
   
   tr->delete_by_index(pred, m, alloc, gc_nodes);

#ifdef COMPILED
#ifndef COMPILED_NO_AGGREGATES
   auto *agg(aggs[pred->get_id()]);
   if(agg) {
      agg->delete_by_index(m, gc_nodes);
   }
#endif
#else
   auto it(aggs.find(pred->get_id()));
   
   if(it != aggs.end()) {
      tuple_aggregate *agg(it->second);
      agg->delete_by_index(m, alloc, gc_nodes);
   }
#endif
}

size_t
persistent_store::count_total(const predicate* pred) const
{
   tuple_trie *tr(get_storage(pred));

   if(pred->is_compact_pred())
      return ((db::array*)tr)->size();
   else
      return tr->size();
}

void
persistent_store::wipeout(mem::node_allocator *alloc, candidate_gc_nodes& gc_nodes)
{
   for(auto *pred : vm::theProgram->persistent_predicates) {
      tuple_trie *tr(get_storage(pred));
      if(pred->is_compact_pred())
         ((array*)tr)->wipeout(pred, gc_nodes);
      else
         tr->wipeout(pred, alloc, gc_nodes);
#ifndef COMPILED
      mem::allocator<tuple_trie>().destroy(tr);
#endif
   }
#ifdef COMPILED
#ifndef COMPILED_NO_AGGREGATES
   for(size_t i(0); i < COMPILED_NUM_TRIES; ++i) {
      tuple_aggregate *agg(aggs[i]);
      if(agg) {
         agg->wipeout(gc_nodes);
         delete agg;
      }
   }
#endif
#else
   if(theProgram->has_aggregates()) {
      for(auto & elem : aggs) {
         tuple_aggregate *agg(elem.second);
         agg->wipeout(alloc, gc_nodes);
         delete agg;
      }
   }
#endif
#ifndef COMPILED
   mem::allocator<tuple_trie>().deallocate(tuples, vm::theProgram->num_persistent_predicates());
#endif
}

vector<string>
persistent_store::dump(const predicate *pred) const
{
   tuple_trie *tr(get_storage(pred));
   if(pred->is_compact_pred()) {
      array *a((array*)tr);
      return a->get_print_strings(pred);
   } else {
      if(!tr->empty())
         return tr->get_print_strings(pred);
   }
   return vector<string>();
}

vector<string>
persistent_store::print(const predicate *pred) const
{
   tuple_trie *tr(get_storage(pred));
   if(pred->is_compact_pred())
      return ((array*)tr)->get_print_strings(pred);
   else {
      if(!tr->empty())
            return tr->get_print_strings(pred);
   }
   return vector<string>();
}
