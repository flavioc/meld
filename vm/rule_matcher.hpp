
#ifndef VM_RULE_MATCHER_HPP
#define VM_RULE_MATCHER_HPP

#include "vm/predicate.hpp"
#include "vm/tuple.hpp"
#include "vm/full_tuple.hpp"
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

	void register_predicate_unavailability(const predicate *pred) {
      for(predicate::rule_iterator it(pred->begin_rules()), end(pred->end_rules()); it != end; it++) {
         vm::rule_id rule_id(*it);
         vm::rule *rule(theProgram->get_rule(rule_id));

         if(rule->as_persistent())
            continue;

         if(rules[rule_id] == rule->num_predicates())
            rule_queue.unset_bit(rule_id);

         assert(rules[rule_id] > 0);
         rules[rule_id]--;
      }
   }

	void register_predicate_availability(const predicate *pred) {
      for(predicate::rule_iterator it(pred->begin_rules()), end(pred->end_rules()); it != end; it++) {
         vm::rule_id rule_id(*it);
         vm::rule *rule(theProgram->get_rule(rule_id));

         if(rule->as_persistent())
            continue;

         rules[rule_id]++;
         assert(rules[rule_id] <= rule->num_predicates());
         if(rules[rule_id] == rule->num_predicates())
            rule_queue.set_bit(rule_id);
      }
   }

public:

   inline void register_predicate_update(const predicate *pred)
   {
      assert(predicate_count[pred->get_id()] > 0);
      for(predicate::rule_iterator it(pred->begin_rules()), end(pred->end_rules()); it != end; it++) {
         vm::rule_id rule_id(*it);
         vm::rule *rule(theProgram->get_rule(rule_id));

         if(rule->as_persistent())
            continue;

         if(rules[rule_id] == rule->num_predicates())
            rule_queue.set_bit(rule_id);
      }
   }

   bitmap rule_queue; // rules next to run

   // returns true if we did not have any tuples of this predicate
	bool register_tuple(const predicate *pred, const derivation_count count,
         const bool is_new = true)
   {
      const vm::predicate_id id(pred->get_id());
      bool ret(false);
      assert(count > 0);

      if(is_new) {
         if(predicate_count[id] == 0) {
            ret = true;
            register_predicate_availability(pred);
         } else
            register_predicate_update(pred);
      }
      predicate_count[id] += count;
      return ret;
   }
   
   // returns true if now we do not have any tuples of this predicate
   bool deregister_tuple(const predicate *pred, const derivation_count count)
   {
      const vm::predicate_id id(pred->get_id());
      bool ret(false);

      assert(count > 0);
      assert(predicate_count[id] >= (pred_count)count);

      predicate_count[id] -= count;
      if(predicate_count[id] == 0) {
         ret = true;
         register_predicate_unavailability(pred);
      }

      return ret;
   }

   inline void register_full_tuple(const full_tuple *stpl)
   {
      register_tuple(stpl->get_predicate(), stpl->get_count());
   }

   inline void deregister_full_tuple(const full_tuple *stpl)
   {
      deregister_tuple(stpl->get_predicate(), stpl->get_count());
   }

   // force a count of predicates
   void set_count(const predicate *pred, const pred_count count)
   {
      const vm::predicate_id id(pred->get_id());
      assert(count >= 0);
      if(predicate_count[id] == 0) {
         if(count > 0)
            register_predicate_availability(pred);
      } else {
         // > 0
         if(count == 0)
            register_predicate_unavailability(pred);
         else
            register_predicate_update(pred);
      }
      predicate_count[id] = count;
   }

   // check if the counters are totally empty.
   inline bool is_empty(void) const {
      for(size_t i(0); i < theProgram->num_predicates(); ++i) {
         if(predicate_count[i])
            return false;
      }
      return true;
   }

   inline pred_count get_count(const vm::predicate_id id) const
   {
      return predicate_count[id];
   }

   rule_matcher(void);
   ~rule_matcher(void);
};

}

#endif
