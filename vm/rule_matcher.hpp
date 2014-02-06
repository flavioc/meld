
#ifndef VM_RULE_MATCHER_HPP
#define VM_RULE_MATCHER_HPP

#include "vm/predicate.hpp"
#include "vm/tuple.hpp"
#include "vm/bitmap.hpp"

namespace vm
{

struct rule_matcher
{
private:
	
   typedef unsigned short pred_count;
   utils::byte *rules; // availability statistics per rule
   pred_count *predicate_count; // number of tuples per predicate

   bitmap *active_bitmap;
   bitmap *dropped_bitmap;
	
	void register_predicate_availability(const predicate *);
	void register_predicate_unavailability(const predicate *);

public:

   // returns true if we did not have any tuples of this predicate
	bool register_tuple(tuple *, const derivation_count, const bool is_new = true);

	// returns true if now we do not have any tuples of this predicate
	bool deregister_tuple(tuple *, const derivation_count);

	void clear_dropped_rules(void)
	{
      dropped_bitmap->clear(theProgram->num_rules_next_uint());
	}

	//typedef set_rules::const_iterator rule_iterator;

   bitmap::iterator active_rules_iterator(void) const { return active_bitmap->begin(theProgram->num_rules_next_uint()); }
   bitmap::iterator dropped_rules_iterator(void) const { return dropped_bitmap->begin(theProgram->num_rules_next_uint()); }
	
   rule_matcher(void);
   ~rule_matcher(void);
};

}

#endif
