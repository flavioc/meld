
#ifndef VM_RULE_HPP
#define VM_RULE_HPP

#include <string>
#include <vector>
#include <ostream>

#include "conf.hpp"
#include "vm/defs.hpp"
#include "vm/predicate.hpp"

namespace vm
{

class rule
{
   private:

      const std::string str;
      byte_code code;
		code_size_t code_size;
      std::vector<predicate*> predicates;

   public:
	
		void print(std::ostream&) const;
		
		inline const std::string get_string(void) const { return str; }

      inline void add_predicate(predicate *p) { predicates.push_back(p); }
      inline void set_bytecode(code_size_t _size, byte_code _code) {
			code_size = _size;
			code = _code;
		}
		
		byte_code get_bytecode(void) const { return code; }

      explicit rule(const std::string& _str): str(_str) { }

		~rule(void) { delete []code; }
};

}

#endif
