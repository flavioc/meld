
#ifndef DB_AGG_CONFIGURATION_HPP
#define DB_AGG_CONFIGURATION_HPP

#include <ostream>

#include "mem/base.hpp"
#include "db/tuple.hpp"
#include "vm/defs.hpp"
#include "vm/types.hpp"
#include "db/trie.hpp"

namespace db
{
   
class agg_configuration: public mem::base
{
private:
   
   typedef tuple_trie::iterator iterator;
   typedef tuple_trie::const_iterator const_iterator;

   bool changed;
   vm::tuple *corresponds;
   vm::depth_t last_depth;
   
   vm::tuple *generate_max_int(vm::predicate *, const vm::field_num, vm::depth_t&) const;
   vm::tuple *generate_min_int(vm::predicate *, const vm::field_num, vm::depth_t&) const;
   vm::tuple *generate_sum_int(vm::predicate *, const vm::field_num, vm::depth_t&) const;
   vm::tuple *generate_sum_float(vm::predicate *, const vm::field_num, vm::depth_t&) const;
   vm::tuple *generate_first(vm::predicate *, vm::depth_t&) const;
   vm::tuple *generate_max_float(vm::predicate *, const vm::field_num, vm::depth_t&) const;
   vm::tuple *generate_min_float(vm::predicate *, const vm::field_num, vm::depth_t&) const;
   vm::tuple *generate_sum_list_float(vm::predicate *, const vm::field_num, vm::depth_t&) const;
   
protected:
   
   tuple_trie vals;

   virtual vm::tuple *do_generate(vm::predicate *, const vm::aggregate_type, const vm::field_num, vm::depth_t&);

public:

   MEM_METHODS(agg_configuration)

   void print(std::ostream&) const;

   void generate(vm::predicate *, const vm::aggregate_type, const vm::field_num, simple_tuple_list&);

   bool test(vm::predicate *, vm::tuple *, const vm::field_num) const;

   inline bool has_changed(void) const { return changed; }
   inline bool is_empty(void) const { return vals.empty(); }
   inline size_t size(void) const { return vals.size(); }

   virtual void add_to_set(vm::tuple *, vm::predicate *, const vm::derivation_count, const vm::depth_t);
   
   bool matches_first_int_arg(vm::predicate *, const vm::int_val) const;

   explicit agg_configuration(vm::predicate *_pred):
      changed(false), corresponds(NULL), last_depth(0), vals(_pred)
   {
      assert(corresponds == NULL);
      assert(!changed);
   }

   virtual ~agg_configuration(void);
};

std::ostream& operator<<(std::ostream&, const agg_configuration&);
}

#endif
