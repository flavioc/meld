
#ifndef VM_RULE_MATCHER_HPP
#define VM_RULE_MATCHER_HPP

#include "vm/predicate.hpp"
#include "vm/tuple.hpp"
#include "vm/bitmap.hpp"
#include "vm/all.hpp"

namespace vm
{

struct rule_matcher
{
private:
	
   typedef unsigned short pred_count;
   utils::byte *rules; // availability statistics per rule
   pred_count *predicate_count; // number of tuples per predicate

	void register_predicate_availability(const predicate *);
	void register_predicate_unavailability(const predicate *);

public:

   bitmap active_bitmap; // rules that may run
   bitmap dropped_bitmap; // rules that are no longer runnable
   bitmap predicates; // new generated predicates

   // returns true if we did not have any tuples of this predicate
	bool register_tuple(predicate *, const derivation_count,
         const bool is_new = true);

	// returns true if now we do not have any tuples of this predicate
	bool deregister_tuple(predicate *, const derivation_count);

   inline pred_count get_count(const vm::predicate_id id) const
   {
      return predicate_count[id];
   }

   inline void mark(const vm::predicate *pred)
   {
      predicates.set_bit(pred->get_id());
   }

   inline void clear_predicates(void)
   {
      predicates.clear(theProgram->num_predicates_next_uint());
   }

	void clear_dropped_rules(void)
	{
      dropped_bitmap.clear(theProgram->num_rules_next_uint());
	}

   bitmap::iterator active_rules_iterator(void) {
      return active_bitmap.begin(theProgram->num_rules());
   }

   bitmap::iterator dropped_rules_iterator(void) {
      return dropped_bitmap.begin(theProgram->num_rules());
   }
	
   rule_matcher(void);
   ~rule_matcher(void);
};

}

#endif
