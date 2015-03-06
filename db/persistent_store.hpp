
#ifndef DB_PERSISTENT_STORE_HPP
#define DB_PERSISTENT_STORE_HPP

#include <unordered_map>

#include "db/trie.hpp"
#include "vm/predicate.hpp"
#include "db/tuple_aggregate.hpp"
#ifdef COMPILED
#include COMPILED_HEADER
#endif

namespace db {

struct persistent_store {
   using delete_info = trie::delete_info;

   using aggregate_map = std::unordered_map<
       vm::predicate_id, tuple_aggregate *, std::hash<vm::predicate_id>,
       std::equal_to<vm::predicate_id>,
       mem::allocator<std::pair<const vm::predicate_id, tuple_aggregate *>>>;

// tuple database
#ifdef COMPILED
   tuple_trie tuples[COMPILED_NUM_TRIES];
#else
   tuple_trie *tuples{nullptr};
#endif

   // sets of tuple aggregates
   aggregate_map aggs;

   tuple_trie *get_storage(const vm::predicate *) const;

   bool add_tuple(vm::tuple *, vm::predicate *, const vm::depth_t);
   delete_info delete_tuple(vm::tuple *, vm::predicate *, const vm::depth_t);

   db::agg_configuration *add_agg_tuple(
       vm::tuple *, vm::predicate *, const vm::depth_t,
       vm::candidate_gc_nodes &,
       const vm::derivation_direction dir = vm::POSITIVE_DERIVATION);
   db::agg_configuration *remove_agg_tuple(vm::tuple *, vm::predicate *,
                                           const vm::depth_t,
                                           vm::candidate_gc_nodes &);
   void delete_by_index(vm::predicate *, const vm::match &,
                        vm::candidate_gc_nodes &);
   void delete_by_leaf(vm::predicate *, tuple_trie_leaf *, const vm::depth_t,
                       vm::candidate_gc_nodes &);

   db::tuple_trie::tuple_iterator match_predicate(
       const vm::predicate_id id) const {
      tuple_trie *tr((tuple_trie *)(tuples + id));

      return tr->match_predicate();
   }

   inline db::tuple_trie::tuple_search_iterator match_predicate(
       const vm::predicate_id id, const vm::match *m) const {
      tuple_trie *tr = (tuple_trie *)(tuples + id);
      return tr->match_predicate(m);
   }

   vm::full_tuple_list end_iteration();

   size_t count_total(const vm::predicate *) const;

   explicit persistent_store();

   void wipeout(vm::candidate_gc_nodes &);

   std::vector<std::string> dump(const vm::predicate *) const;
   std::vector<std::string> print(const vm::predicate *) const;
};
}

#endif
