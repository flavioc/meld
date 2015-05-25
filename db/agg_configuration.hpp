
#ifndef DB_AGG_CONFIGURATION_HPP
#define DB_AGG_CONFIGURATION_HPP

#include <ostream>

#include "mem/base.hpp"
#include "vm/full_tuple.hpp"
#include "vm/defs.hpp"
#include "vm/types.hpp"
#include "db/trie.hpp"
#include "mem/node.hpp"

namespace db
{
   
class agg_configuration: public mem::base
{
private:
   
   using iterator = tuple_trie::iterator;
   using const_iterator = tuple_trie::const_iterator;

   bool changed;
   vm::tuple *corresponds;
   vm::depth_t last_depth;
   
   vm::tuple *generate_max_int(vm::predicate *, const vm::field_num, vm::depth_t&, mem::node_allocator *) const;
   vm::tuple *generate_min_int(vm::predicate *, const vm::field_num, vm::depth_t&, mem::node_allocator *) const;
   vm::tuple *generate_sum_int(vm::predicate *, const vm::field_num, vm::depth_t&, mem::node_allocator *) const;
   vm::tuple *generate_sum_float(vm::predicate *, const vm::field_num, vm::depth_t&, mem::node_allocator *) const;
   vm::tuple *generate_first(vm::predicate *, vm::depth_t&, mem::node_allocator *) const;
   vm::tuple *generate_max_float(vm::predicate *, const vm::field_num, vm::depth_t&, mem::node_allocator *) const;
   vm::tuple *generate_min_float(vm::predicate *, const vm::field_num, vm::depth_t&, mem::node_allocator *) const;
   vm::tuple *generate_sum_list_float(vm::predicate *, const vm::field_num, vm::depth_t&, mem::node_allocator *) const;
   
protected:
   
   tuple_trie vals;

   virtual vm::tuple *do_generate(vm::predicate *, const vm::aggregate_type,
         const vm::field_num, vm::depth_t&, mem::node_allocator *);

public:

   void print(std::ostream&, vm::predicate *) const;

   void generate(vm::predicate *, const vm::aggregate_type, const vm::field_num,
         vm::full_tuple_list&, mem::node_allocator *);

   bool test(vm::predicate *, vm::tuple *, const vm::field_num) const;

   inline bool has_changed(void) const { return changed; }
   inline bool is_empty(void) const { return vals.empty(); }
   inline size_t size(void) const { return vals.size(); }

   virtual void add_to_set(vm::tuple *, vm::predicate *, const vm::derivation_direction,
         const vm::depth_t, mem::node_allocator *, vm::candidate_gc_nodes&);
   
   bool matches_first_int_arg(vm::predicate *, const vm::int_val) const;

   explicit agg_configuration(void):
      changed(false), corresponds(nullptr), last_depth(0)
   {
      assert(corresponds == nullptr);
      assert(!changed);
   }

   inline void wipeout(vm::predicate *pred, mem::node_allocator *alloc, vm::candidate_gc_nodes& gc_nodes)
   {
      vals.wipeout(pred, alloc, gc_nodes);
      if(corresponds) {
         vm::tuple::destroy(corresponds, pred, alloc, gc_nodes);
         corresponds = nullptr;
      }
   }
};
}

#endif
