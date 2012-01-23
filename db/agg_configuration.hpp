
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
   
   vm::tuple *generate_max_int(const vm::field_num) const;
   vm::tuple *generate_min_int(const vm::field_num) const;
   vm::tuple *generate_sum_int(const vm::field_num) const;
   vm::tuple *generate_sum_float(const vm::field_num) const;
   vm::tuple *generate_first(void) const;
   vm::tuple *generate_max_float(const vm::field_num) const;
   vm::tuple *generate_min_float(const vm::field_num) const;
   vm::tuple *generate_sum_list_float(const vm::field_num) const;
   
protected:
   
   tuple_trie vals;

   virtual vm::tuple *do_generate(const vm::aggregate_type, const vm::field_num);

public:

   MEM_METHODS(agg_configuration)

   void print(std::ostream&) const;

   void generate(const vm::aggregate_type, const vm::field_num, simple_tuple_list&);

   bool test(vm::tuple *, const vm::field_num) const;

   inline bool has_changed(void) const { return changed; }
   inline bool is_empty(void) const { return vals.empty(); }
   inline size_t size(void) const { return vals.size(); }

   virtual void add_to_set(vm::tuple *, const vm::ref_count);
   
   bool matches_first_int_arg(const vm::int_val) const;

   explicit agg_configuration(const vm::predicate *_pred):
      changed(false), corresponds(NULL), vals(_pred)
   {
      assert(corresponds == NULL);
      assert(!changed);
   }

   virtual ~agg_configuration(void);
};

std::ostream& operator<<(std::ostream&, const agg_configuration&);
}

#endif
