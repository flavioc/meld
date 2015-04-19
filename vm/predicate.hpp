
#ifndef PREDICATE_HPP
#define PREDICATE_HPP

#include <string>
#include <vector>
#include <assert.h>

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "utils/types.hpp"
#include "vm/reader.hpp"
#include "vm/bitmap.hpp"

namespace vm {

const size_t PRED_DESCRIPTOR_BASE_SIZE = 4;
const size_t PRED_ARGS_MAX = 32;
const size_t PRED_NAME_SIZE_MAX = 32;
const size_t PRED_AGG_INFO_MAX = 32;

class program;

typedef enum { LINKED_LIST, HASH_TABLE } store_type_t;

class rule;

struct predicate {
   public:
   static predicate_id current_id;

   predicate_id id;
   predicate_id id2; // persistent or linear ID
   std::string name;
   strat_level level{0};

   std::vector<type*> types;
   std::vector<size_t> fields_size;
   std::vector<size_t> fields_offset;

   size_t tuple_size;

   typedef struct {
      field_num field;
      aggregate_type type;
      aggregate_safeness safeness;
      strat_level local_level;
      predicate_id remote_pred_id;
      predicate* remote_pred;
   } aggregate_info;

   aggregate_info* agg_info;

   bool is_route{false};
   bool is_linear{false};
   bool is_reverse_route{false};
   bool is_action{false};
   bool is_reused{false};
   bool is_cycle{false};
   bool is_thread{false};
   bool is_compact{false};
   bool has_code{true};

   std::vector<const rule*, mem::allocator<const rule*>> affected_rules;
   std::vector<const rule*, mem::allocator<const rule*>> linear_rules;

   inline void add_linear_affected_rule(const rule* rule) {
      affected_rules.push_back(rule);
      linear_rules.push_back(rule);
   }

   store_type_t store_type;
   field_num hash_argument;

   // index of this predicate's arguments in the whole set of program's
   // predicates
   size_t argument_position;

   void build_field_info(void);
   void build_aggregate_info(vm::program*);
   void cache_info(vm::program*);

   explicit predicate(void);

   void destroy();
   ~predicate(void);

   using rule_iterator =
       std::vector<const rule*, mem::allocator<const rule*>>::const_iterator;

   inline rule_iterator begin_rules(void) const {
      return affected_rules.begin();
   }
   inline rule_iterator end_rules(void) const { return affected_rules.end(); }

   inline rule_iterator begin_linear_rules() const {
      return linear_rules.begin();
   }
   inline rule_iterator end_linear_rules() const { return linear_rules.end(); }

   inline bool is_aggregate_pred(void) const { return agg_info != nullptr; }

   inline bool is_compact_pred() const { return is_compact; }

   inline aggregate_safeness get_agg_safeness(void) const {
      return agg_info->safeness;
   }
   inline bool is_unsafe_agg(void) const {
      return get_agg_safeness() == AGG_UNSAFE ||
             get_agg_safeness() == AGG_IMMEDIATE;
   }
   inline const predicate* get_remote_pred(void) const {
      return agg_info->remote_pred;
   }
   strat_level get_agg_strat_level(void) const { return agg_info->local_level; }

   inline bool is_route_pred(void) const {
      return is_route || is_reverse_route;
   }
   inline bool is_thread_pred() const { return is_thread; }

   inline bool is_reverse_route_pred(void) const { return is_reverse_route; }

   inline bool is_linear_pred(void) const { return is_linear; }
   inline bool is_persistent_pred(void) const { return !is_linear_pred(); }

   inline bool is_action_pred(void) const { return is_action; }

   inline bool is_reused_pred(void) const { return is_reused; }

   inline bool is_cycle_pred(void) const { return is_cycle; }

   inline field_num get_aggregate_field(void) const { return agg_info->field; }
   inline aggregate_type get_aggregate_type(void) const {
      return agg_info->type;
   }

   inline predicate_id get_id(void) const { return id; }
   inline predicate_id get_persistent_id(void) const { return id2; }
   inline predicate_id get_linear_id(void) const { return id2; }

   inline size_t num_fields(void) const { return types.size(); }

   inline type* get_field_type(const field_num field) const {
      return types[field];
   }
   inline size_t get_field_size(const field_num field) const {
      return fields_size[field];
   }

   inline std::string get_name(void) const { return name; }

   inline size_t get_size(void) const { return tuple_size; }

   inline strat_level get_strat_level(void) const { return level; }

   inline void store_as_hash_table(const field_num field) {
      store_type = HASH_TABLE;
      hash_argument = field;
   }

   inline field_num get_hashed_field(void) const {
      assert(is_hash_table());
      return hash_argument;
   }

   inline bool is_hash_table(void) const { return store_type == HASH_TABLE; }

   inline void set_argument_position(const size_t arg) {
      argument_position = arg;
   }

   inline size_t get_argument_position(void) const { return argument_position; }

   void print_simple(std::ostream&) const;
   void print(std::ostream&) const;

   bool operator==(const predicate&) const;
   bool operator!=(const predicate& other) const { return !operator==(other); }

   static predicate* make_predicate_from_reader(
       code_reader&, code_size_t*, const predicate_id, const uint32_t,
       const uint32_t, const std::vector<type*, mem::allocator<type*>>&);

   static predicate* make_predicate_simple(const predicate_id,
                                           const std::string&, const bool,
                                           const std::vector<type*>&);
};

type* read_type_from_reader(code_reader&, vm::program*);
type* read_type_id_from_reader(
    code_reader&, const std::vector<type*, mem::allocator<type*>>&);
std::ostream& operator<<(std::ostream&, const predicate&);
}

#endif
