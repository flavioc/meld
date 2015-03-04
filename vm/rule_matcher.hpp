
#ifndef VM_RULE_MATCHER_HPP
#define VM_RULE_MATCHER_HPP

#include "vm/predicate.hpp"
#include "vm/tuple.hpp"
#include "vm/full_tuple.hpp"
#include "vm/bitmap.hpp"
#include "vm/all.hpp"
#include "db/linear_store.hpp"

namespace vm {

struct rule_matcher {
   private:
   utils::byte *rules;  // availability statistics per rule
   bitmap predicate_existence;

   void register_predicate_unavailability(const predicate *pred) {
      predicate_existence.unset_bit(pred->get_id());

      for (auto it(pred->begin_linear_rules()), end(pred->end_linear_rules());
           it != end; it++) {
         const vm::rule *rule(*it);

         if (rules[rule->get_id()] == rule->num_predicates())
            rule_queue.unset_bit(rule->get_id());

         assert(rules[rule->get_id()] > 0);
         rules[rule->get_id()]--;
      }
   }

   void register_predicate_availability(const predicate *pred) {
      predicate_existence.set_bit(pred->get_id());
      for (auto it(pred->begin_linear_rules()), end(pred->end_linear_rules());
           it != end; it++) {
         const vm::rule *rule(*it);

         rules[rule->get_id()]++;
         assert(rules[rule->get_id()] <= rule->num_predicates());
         if (rules[rule->get_id()] == rule->num_predicates())
            rule_queue.set_bit(rule->get_id());
      }
   }

   public:
   bitmap rule_queue;  // rules next to run

   inline void register_predicate_update(const predicate *pred) {
      assert(predicate_existence.get_bit(pred->get_id()));

      for (auto it(pred->begin_linear_rules()), end(pred->end_linear_rules());
           it != end; it++) {
         const vm::rule *rule(*it);

         if (rules[rule->get_id()] == rule->num_predicates())
            rule_queue.set_bit(rule->get_id());
      }
   }

   inline void new_linear_fact(const vm::predicate *pred) {
      if (predicate_existence.get_bit(pred->get_id()))
         register_predicate_update(pred);
      else
         register_predicate_availability(pred);
   }

   inline void new_persistent_fact(const vm::predicate *pred) {
      new_linear_fact(pred);
   }

   inline void new_persistent_count(const vm::predicate *pred,
                                    const size_t count) {
      if (count == 0) {
         if (predicate_existence.get_bit(pred->get_id()))
            register_predicate_unavailability(pred);
      } else if (count > 0)
         new_persistent_fact(pred);
   }

   inline void empty_predicate(const vm::predicate *pred) {
      if (predicate_existence.get_bit(pred->get_id()))
         register_predicate_unavailability(pred);
   }

   inline bool is_empty(void) const {
      return predicate_existence.empty(theProgram->num_predicates_next_uint());
   }

   inline void add_thread(rule_matcher &thread_matcher) {
      // adds facts marked in the thread matcher
      for (auto it(thread_matcher.predicate_existence.begin(
               theProgram->num_predicates()));
           !it.end(); ++it) {
         const size_t pred(*it);
         new_linear_fact(theProgram->get_predicate(pred));
      }
   }

   inline void print(std::ostream &cout) {
      for (auto it(predicate_existence.begin(theProgram->num_predicates()));
           !it.end(); ++it) {
         vm::predicate *pred(theProgram->get_predicate(*it));
         cout << "Has predicate " << pred->get_name() << "\n";
      }
   }

   inline void remove_thread(rule_matcher &thread_matcher) {
      thread_matcher.predicate_existence.set_bits_of_and_result(
          theProgram->thread_predicates_map, predicate_existence,
          theProgram->num_predicates_next_uint());
      for (auto it(theProgram->thread_predicates_map.begin(
               theProgram->num_predicates()));
           !it.end(); ++it) {
         const size_t id(*it);
         if (predicate_existence.get_bit(id))
            register_predicate_unavailability(theProgram->get_predicate(*it));
      }
   }

   rule_matcher(void);
   ~rule_matcher(void);
};
}

#endif
