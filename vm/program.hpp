
#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <ostream>

#include "vm/predicate.hpp"
#include "vm/defs.hpp"
#include "vm/tuple.hpp"
#include "vm/rule.hpp"
#include "vm/function.hpp"
#include "runtime/objs.hpp"
#include "queue/heap_implementation.hpp"
#include "vm/import.hpp"
#include "vm/bitmap.hpp"
#include "vm/bitmap_static.hpp"
#include "vm/special_facts.hpp"
#ifdef USE_REAL_NODES
#include <unordered_map>
#endif

namespace db {
class database;
struct node;
}

namespace vm {

typedef enum { PRIORITY_ASC, PRIORITY_DESC } priority_type;

const size_t INIT_PREDICATE_ID(0);
const size_t INIT_THREAD_PREDICATE_ID(1);

#define VERSION_AT_LEAST(MAJ, MIN) \
   (major_version > (MAJ) || (major_version == (MAJ) && minor_version >= (MIN)))

class program {
   public:
   const std::string filename;
   uint32_t major_version, minor_version;

#ifdef COMPILED
   vm::type *types[COMPILED_NUM_TYPES];
#else
   std::vector<type *, mem::allocator<type *>> types;
#endif

   std::vector<import *, mem::allocator<import *>> imported_predicates;
   std::vector<std::string, mem::allocator<std::string>> exported_predicates;

   size_t num_args{0};
   size_t number_rules{0};
   size_t number_rules_uint{0};

   std::vector<rule *, mem::allocator<rule *>> rules;

   std::vector<function *, mem::allocator<function *>> functions;

#ifdef COMPILED
   vm::predicate predicates[COMPILED_NUM_PREDICATES];
   vm::predicate *linear_predicates[COMPILED_NUM_LINEAR];
   vm::predicate *persistent_predicates[COMPILED_NUM_TRIES];
#else
   std::vector<predicate *, mem::allocator<predicate *>> predicates;
   std::vector<predicate *, mem::allocator<predicate *>> persistent_predicates;
   std::vector<predicate *, mem::allocator<predicate *>> linear_predicates;

   std::vector<byte_code, mem::allocator<byte_code>> code;
   std::vector<code_size_t, mem::allocator<code_size_t>> code_size;
#endif
   std::vector<predicate *, mem::allocator<predicate *>> sorted_predicates;
   size_t num_predicates_uint{0};
   size_t num_linear_predicates_uint{0};

   code_size_t const_code_size{0};
   byte_code const_code{nullptr};
   std::vector<type *, mem::allocator<type *>> const_types;

   std::vector<predicate *, mem::allocator<predicate *>> route_predicates;
   // predicates that are instantiated at the thread level
   std::vector<predicate *, mem::allocator<predicate *>> thread_predicates;

   bool has_aggregates_flag{false};
   bool safe{true};
   bool is_data_file{false};

   rule *data_rule;

   mutable predicate *init{nullptr}, *init_thread{nullptr};

   special_facts_id special;

   using string_store = std::vector<runtime::rstring::ptr,
                                    mem::allocator<runtime::rstring::ptr>>;

   string_store default_strings;

   strat_level priority_strat_level{0};
   vm::priority_type priority_order;
   bool priority_static{false};
   vm::priority_t initial_priority;
   vm::priority_t default_priority;
   vm::priority_t no_priority;

#ifndef COMPILED
#ifdef USE_REAL_NODES
   using node_ref_vector = std::vector<byte_code, mem::allocator<byte_code>>;
   // node references in the byte code
   node_ref_vector *node_references{nullptr};
   // node references in constant code
   using const_ref_vector =
      std::vector<std::pair<vm::node_val, byte_code>,
                  mem::allocator<std::pair<vm::node_val, byte_code>>>;
   const_ref_vector const_node_references;
#endif
#endif

   size_t total_arguments{0};

   void read_node_references(byte_code, code_reader &);
   void const_read_node_references(byte_code, code_reader &);

#ifdef COMPILED
   vm::bitmap_static<COMPILED_NUM_PREDICATES_UINT> thread_predicates_map;
#else
   vm::bitmap thread_predicates_map;
#endif

   strat_level MAX_STRAT_LEVEL;

   inline size_t num_types(void) const {
#ifdef COMPILED
      return COMPILED_NUM_TYPES;
#else
      return types.size();
#endif
   }

#ifndef COMPILED
   inline void add_type(type *t) { types.push_back(t); }
#endif
   inline type *get_type(const size_t i) const {
      assert(i < num_types());
      return types[i];
   }
#ifndef COMPILED
   inline void add_predicate(vm::predicate *pred) {
      predicates.push_back(pred);
      sorted_predicates.push_back(pred);
   }
#endif
   void sort_predicates();

   inline size_t num_rules(void) const { return number_rules; }
   inline size_t num_rules_next_uint(void) const { return number_rules_uint; }
   inline size_t num_args_needed(void) const { return num_args; }

   inline std::string get_name(void) const { return filename; }

   inline rule *get_rule(const rule_id id) const {
      assert(id < number_rules);
      return rules[id];
   }

   inline function *get_function(const size_t id) const {
      assert(id < functions.size());
      return functions[id];
   }

   inline size_t get_total_arguments(void) const { return total_arguments; }

   inline strat_level get_priority_strat_level(void) const {
      return priority_strat_level;
   }
   inline bool is_priority_asc(void) const {
      return priority_order == PRIORITY_ASC;
   }
   inline bool is_priority_desc(void) const {
      return priority_order == PRIORITY_DESC;
   }

   inline vm::priority_t get_initial_priority(void) const { return initial_priority; }
   inline bool is_static_priority(void) const { return priority_static; }
   inline vm::priority_t get_default_priority() const { return default_priority; }
   inline vm::priority_t get_no_priority_value() const { return no_priority; }

   inline bool has_aggregates() const { return has_aggregates_flag; }

   using predicate_iterator =
       std::vector<predicate *, mem::allocator<predicate *>>::iterator;

   predicate_iterator begin_thread_predicates() {
      return thread_predicates.begin();
   }
   predicate_iterator end_thread_predicates() {
      return thread_predicates.end();
   }

   predicate *get_predicate_by_name(const std::string &) const;

   inline predicate *get_init_predicate(void) const {
      if (init == nullptr) {
         init = get_predicate(INIT_PREDICATE_ID);
         if (init->get_name() != "_init") {
            std::cerr << "_init program should be predicate #"
                 << (int)INIT_PREDICATE_ID << std::endl;
            init = get_predicate_by_name("_init");
         }
         assert(init->get_name() == "_init");
      }

      assert(init != nullptr);

      return init;
   }

   predicate *get_init_thread_predicate(void) const;
   predicate *get_edge_predicate(void) const;

   inline bool has_thread_predicates() const {
      return !thread_predicates.empty();
   }

   inline bool has_special_fact(const vm::special_facts::flag_type f) { return special.has(f); }
   inline vm::predicate *get_special_fact(const vm::special_facts::flag_type f) { return special.get(f); }

#ifndef COMPILED
   void print_predicate_code(std::ostream &, predicate *) const;
   void print_bytecode(std::ostream &) const;
   void print_program(std::ostream &) const;
   void print_bytecode_by_predicate(std::ostream &, const std::string &) const;
#endif
   void print_predicates(std::ostream &) const;
   void print_rules(std::ostream &) const;

   predicate *get_predicate(const predicate_id i) const {
#ifdef COMPILED
      return (predicate*)predicates + i;
#else
      return predicates[i];
#endif
   }
   predicate *get_linear_predicate(const predicate_id i) const {
      return linear_predicates[i];
   }
   predicate *get_persistent_predicate(const predicate_id i) const {
      return persistent_predicates[i];
   }
   predicate *get_sorted_predicate(const size_t i) const {
      assert(i < num_predicates());
      return sorted_predicates[i];
   }
   predicate *get_route_predicate(const size_t &) const;

#ifdef COMPILED
   inline size_t num_predicates() const { return COMPILED_NUM_PREDICATES; }
   inline size_t num_persistent_predicates() const {
      return COMPILED_NUM_TRIES;
   }
   inline size_t num_linear_predicates() const {
      return COMPILED_NUM_LINEAR;
   }
#else
   inline size_t num_predicates() const { return predicates.size(); }

   byte_code get_predicate_bytecode(const predicate_id id) const {
      assert(id < num_predicates());
      return code[id];
   }
   inline byte_code get_const_bytecode() const { return const_code; }
   inline type *get_const_type(const const_id &id) const {
      return const_types[id];
   }
   size_t num_persistent_predicates() const {
      return persistent_predicates.size();
   }
   size_t num_linear_predicates() const { return linear_predicates.size(); }
#endif
   inline bool has_const_code() const { return const_code_size != 0; }
   size_t num_route_predicates() const { return route_predicates.size(); }
   size_t num_thread_predicates() const { return thread_predicates.size(); }
   size_t num_predicates_next_uint() const { return num_predicates_uint; }
   size_t num_linear_predicates_next_uint() const {
      return num_linear_predicates_uint;
   }

   inline runtime::rstring::ptr get_default_string(const size_t i) const {
      assert(i < default_strings.size());
      return default_strings[i];
   }

   inline bool is_safe(void) const { return safe; }
   inline bool is_data(void) const { return is_data_file; }

   bool add_data_file(vm::program &);
   void fix_node_address(db::node *);
#ifndef COMPILED
   void fix_const_references();
   void cleanup_node_references() { if(node_references) delete []node_references; }
#endif

   static std::unique_ptr<std::ifstream> bypass_bytecode_header(
       const std::string &);

#ifndef COMPILED
   explicit program(std::string);
#endif
   explicit program(void);  // add compiled program

   ~program(void);
};
}

#endif
