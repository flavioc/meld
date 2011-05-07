
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
   
class program
{
private:
   
   std::vector<predicate*> predicates;
  
   std::vector<byte_code> code;
   std::vector<code_size_t> code_size;
   
public:
   
   predicate *get_predicate_by_name(const std::string&) const;
   
   predicate *get_init_predicate(void) const;
   
   predicate *get_edge_predicate(void) const;
   
   void print_bytecode(std::ostream&) const;
   
   predicate* get_predicate(const predicate_id&) const;
   
   byte_code get_bytecode(const predicate_id&) const;
   
   size_t num_predicates(void) const { return predicates.size(); }
   
   tuple* new_tuple(const predicate_id&) const;
   
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