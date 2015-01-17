
#ifndef VM_RULE_HPP
#define VM_RULE_HPP

#include <string>
#include <vector>
#include <ostream>

#include "vm/defs.hpp"

namespace vm
{

class program;

class rule
{
   private:

      rule_id id;
      const std::string str;
      byte_code code;
		code_size_t code_size;
      using predicate_vector =
		   std::vector<predicate_id, mem::allocator<predicate_id>>;
      predicate_vector predicates;

   public:
	
		void print(std::ostream&, const vm::program * const) const;
		
      inline rule_id get_id(void) const { return id; }
		inline const std::string get_string(void) const { return str; }

      inline void add_predicate(const predicate_id id) {
         predicates.push_back(id);
      }

      inline void set_bytecode(code_size_t _size, byte_code _code) {
			code_size = _size;
			code = _code;
		}
		
      inline code_size_t get_codesize(void) const { return code_size; }
		inline byte_code get_bytecode(void) const { return code; }
		inline size_t num_predicates(void) const { return predicates.size(); }

      void jit_compile();

      explicit rule(const rule_id _id, const std::string& _str):
         id(_id), str(_str)
      {
      }

      inline void destroy()
      {
         delete []code;
      }
};

}

#endif
