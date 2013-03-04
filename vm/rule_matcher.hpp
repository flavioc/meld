
#ifndef VM_RULE_MATCHER_HPP
#define VM_RULE_MATCHER_HPP

#include <vector>
#include <set>

#include "vm/predicate.hpp"
#include "vm/tuple.hpp"

namespace vm
{

class rule_matcher
{
private:
	
#ifdef USE_RULE_COUNTING
   class rule_matcher_obj
   {
      public:
         size_t total_have;
         size_t total_needed;
         bool ignore;
   };

	typedef std::vector<rule_matcher_obj> rule_vector;

   rule_vector rules; /* availability statistics per rule */
   std::vector<ref_count> predicate_count; /* number of tuples per predicate */

	typedef std::set<rule_id, std::less<rule_id>, mem::allocator<rule_id> > set_rules;
	set_rules active_rules; /* rules that *may* be derivable */
	set_rules dropped_rules; /* any dropped rule */
	
	void register_predicate_availability(const predicate *);
	void register_predicate_unavailability(const predicate *);
#endif

public:

#ifdef USE_RULE_COUNTING
   /* returns true if we did not have any tuples of this predicate */
	bool register_tuple(tuple *, const ref_count, const bool is_new = true);

	/* returns true if now we do not have any tuples of this predicate */
	bool deregister_tuple(tuple *, const ref_count);

	void clear_dropped_rules(void)
	{
		dropped_rules.clear();
	}

	typedef set_rules::const_iterator rule_iterator;
	
	rule_iterator begin_active_rules(void) const { return active_rules.begin(); }
	rule_iterator end_active_rules(void) const { return active_rules.end(); }
	rule_iterator begin_dropped_rules(void) const { return dropped_rules.begin(); }
	rule_iterator end_dropped_rules(void) const { return dropped_rules.end(); }
#endif

   rule_matcher(vm::program *);
};

}

#endif
