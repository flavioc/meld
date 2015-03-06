
#ifndef VM_RULE_MATCHER_HPP
#define VM_RULE_MATCHER_HPP

#include "vm/predicate.hpp"
#include "vm/tuple.hpp"
#include "vm/full_tuple.hpp"
#include "vm/bitmap.hpp"
#include "vm/all.hpp"
#include "db/linear_store.hpp"
#ifdef COMPILED
#include COMPILED_HEADER
#endif

namespace vm {

struct rule_matcher {
   private:
   utils::byte *rules;  // availability statistics per rule
   bitmap predicate_existence;

   void register_predicate_unavailability(const predicate_id id) {
      predicate_existence.unset_bit(id);
#ifdef COMPILED
      compiled_register_predicate_unavailability(rules, rule_queue, id);
#else
      for (auto it(theProgram->get_predicate(id)->begin_linear_rules()),
           end(theProgram->get_predicate(id)->end_linear_rules());
           it != end; it++) {
         const vm::rule *rule(*it);

         if (rules[rule->get_id()] == rule->num_predicates())
            rule_queue.unset_bit(rule->get_id());

         assert(rules[rule->get_id()] > 0);
         rules[rule->get_id()]--;
      }
#endif
   }

   void register_predicate_availability(const predicate_id id) {
      predicate_existence.set_bit(id);
#ifdef COMPILED
      compiled_register_predicate_availability(rules, rule_queue, id);
#else
      for (auto it(theProgram->get_predicate(id)->begin_linear_rules()),
           end(theProgram->get_predicate(id)->end_linear_rules());
           it != end; it++) {
         const vm::rule *rule(*it);

         rules[rule->get_id()]++;
         assert(rules[rule->get_id()] <= rule->num_predicates());
         if (rules[rule->get_id()] == rule->num_predicates())
            rule_queue.set_bit(rule->get_id());
      }
#endif
   }

   public:
   bitmap rule_queue;  // rules next to run

   inline void register_predicate_update(const predicate_id id) {
      assert(predicate_existence.get_bit(id));
#ifdef COMPILED
      compiled_register_predicate_update(rules, rule_queue, id);
#else
      for (auto it(theProgram->get_predicate(id)->begin_linear_rules()),
           end(theProgram->get_predicate(id)->end_linear_rules());
           it != end; it++) {
         const vm::rule *rule(*it);

         if (rules[rule->get_id()] == rule->num_predicates())
            rule_queue.set_bit(rule->get_id());
      }
#endif
   }

   inline void new_linear_fact(const vm::predicate_id id) {
      if (predicate_existence.get_bit(id))
         register_predicate_update(id);
      else
         register_predicate_availability(id);
   }

   inline void new_persistent_fact(const vm::predicate_id id) {
      new_linear_fact(id);
   }

   inline void new_persistent_count(const vm::predicate_id id,
                                    const size_t count) {
      if (count == 0) {
         if (predicate_existence.get_bit(id))
            register_predicate_unavailability(id);
      } else if (count > 0)
         new_persistent_fact(id);
   }

   inline void empty_predicate(const vm::predicate_id id) {
      if (predicate_existence.get_bit(id))
         register_predicate_unavailability(id);
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
         new_linear_fact(pred);
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
            register_predicate_unavailability(*it);
      }
   }

   rule_matcher(void);
   ~rule_matcher(void);
};
}

#endif
