
#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include "conf.hpp"

#include <string>
#include <vector>
#include <stdexcept>
#include <ostream>

#include "vm/predicate.hpp"
#include "vm/defs.hpp"
#include "vm/tuple.hpp"
#include "vm/rule.hpp"
#include "runtime/string.hpp"
#include "queue/heap_implementation.hpp"
#ifdef USE_UI
#include <json_spirit.h>
#endif

namespace process {
   class router;
}

namespace vm {

const size_t INIT_PREDICATE_ID(0);
extern size_t SET_PRIORITY_PREDICATE_ID;
extern size_t SETCOLOR_PREDICATE_ID;
extern size_t SETEDGELABEL_PREDICATE_ID;
extern size_t WRITE_STRING_PREDICATE_ID;
extern size_t ADD_PRIORITY_PREDICATE_ID;
extern size_t SCHEDULE_NEXT_PREDICATE_ID;
extern size_t SETCOLOR2_PREDICATE_ID;

class program
{
private:

   const std::string filename;
	size_t num_args;
   size_t number_rules;

   std::vector<rule *> rules;
   
   std::vector<predicate*> predicates;
  
   std::vector<byte_code> code;
   std::vector<code_size_t> code_size;

	code_size_t const_code_size;
	byte_code const_code;
	std::vector<field_type> const_types;
   
   std::vector<predicate*> route_predicates;
   
   bool safe;

   mutable predicate *init;

	typedef std::vector<runtime::rstring::ptr> string_store;
	
	string_store default_strings;
	
	predicate *priority_pred;
   strat_level priority_strat_level;
   field_type priority_type;
   vm::priority_type priority_order;
   heap_priority initial_priority;

   void print_predicate_code(std::ostream&, predicate*) const;
   
public:

   static strat_level MAX_STRAT_LEVEL;

   inline size_t num_rules(void) const { return number_rules; }
	inline size_t num_args_needed(void) const { return num_args; }

   inline std::string get_name(void) const { return filename; }

   inline rule *get_rule(const rule_id id) const {
      assert(id < number_rules);
      return rules[id];
   }

	inline predicate *get_priority_predicate(void) const { return priority_pred; }
	inline field_num get_priority_argument(void) const { return priority_pred->get_priority_argument(); }
   inline field_type get_priority_type(void) const { return priority_type; }
   inline strat_level get_priority_strat_level(void) const { return priority_strat_level; }
	inline bool is_priority_asc(void) const { return priority_order == PRIORITY_ASC; }
	inline bool is_priority_desc(void) const { return priority_order == PRIORITY_DESC; }
	inline bool has_global_priority(void) const { return priority_pred != NULL; }

   inline heap_priority get_initial_priority(void) const { return initial_priority; }
	
   predicate *get_predicate_by_name(const std::string&) const;
   
   predicate *get_init_predicate(void) const;
   
   predicate *get_edge_predicate(void) const;
   
   void print_bytecode(std::ostream&) const;
   void print_predicates(std::ostream&) const;
   void print_bytecode_by_predicate(std::ostream&, const std::string&) const;
#ifdef USE_UI
	json_spirit::Value dump_json(void) const;
#endif
   
   predicate* get_predicate(const predicate_id&) const;
   predicate* get_route_predicate(const size_t&) const;
   
   byte_code get_predicate_bytecode(const predicate_id id) const {
      assert(id < num_predicates());
      return code[id];
   }
	inline byte_code get_const_bytecode(void) const { return const_code; }
	inline field_type get_const_type(const const_id& id) const { return const_types[id]; }
   
   size_t num_predicates(void) const { return predicates.size(); }
   size_t num_route_predicates(void) const { return route_predicates.size(); }

	inline runtime::rstring::ptr get_default_string(const size_t i) const
	{
      assert(i < default_strings.size());
		return default_strings[i];
	}
   
   tuple* new_tuple(const predicate_id&) const;
   
   inline bool is_safe(void) const { return safe; }
   
   explicit program(const std::string&);
   
   ~program(void);
};

class load_file_error : public std::runtime_error {
 public:
    explicit load_file_error(const std::string& filename, const std::string& reason) :
         std::runtime_error(std::string("unable to load byte-code file ") + filename + ": " + reason)
    {}
};

}

#endif
