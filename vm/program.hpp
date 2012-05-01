
#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <ostream>

#include "vm/predicate.hpp"
#include "vm/defs.hpp"
#include "vm/tuple.hpp"

namespace process {
   class router;
}

namespace vm {

const size_t INIT_PREDICATE_ID(0);
const size_t SETPRIO_PREDICATE_ID(1);
   
class program
{
private:
   
   std::vector<predicate*> predicates;
  
   std::vector<byte_code> code;
   std::vector<code_size_t> code_size;
   
   std::vector<predicate*> route_predicates;
   
   bool safe;

   void print_predicate_code(std::ostream&, predicate*) const;
   
public:
   
   predicate *get_predicate_by_name(const std::string&) const;
   
   predicate *get_init_predicate(void) const;
   
   predicate *get_edge_predicate(void) const;
   
   void print_bytecode(std::ostream&) const;
   void print_predicates(std::ostream&) const;
   void print_bytecode_by_predicate(std::ostream&, const std::string&) const;
   
   predicate* get_predicate(const predicate_id&) const;
   predicate* get_route_predicate(const size_t&) const;
   
   byte_code get_bytecode(const predicate_id&) const;
   
   size_t num_predicates(void) const { return predicates.size(); }
   size_t num_route_predicates(void) const { return route_predicates.size(); }
   
   tuple* new_tuple(const predicate_id&) const;
   
   inline bool is_safe(void) const { return safe; }
   
   explicit program(const std::string&);
   
   ~program(void);
};

class load_file_error : public std::runtime_error {
 public:
    explicit load_file_error(const std::string& filename) :
         std::runtime_error(std::string("unable to load byte-code file ") + filename)
    {}
};

}

#endif
