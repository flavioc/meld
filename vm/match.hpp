
#ifndef VM_MATCH_HPP
#define VM_MATCH_HPP

#include <vector>
#include <stack>
#include <ostream>

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "mem/allocator.hpp"
#include "vm/predicate.hpp"
#include "utils/stack.hpp"
#include "vm/types.hpp"

namespace vm
{

struct match_field {
   bool exact;
   type *ty;
	tuple_field field;
};

typedef utils::stack<match_field> match_stack;

class match: public mem::base
{
private:
   
   bool any_exact;
   std::vector<match_field, mem::allocator<match_field> > matches;
   
   inline void set_any_all(const predicate *pred) {
      for(size_t i(0); i < matches.size(); ++i) {
         matches[i].exact = false;
         matches[i].ty = pred->get_field_type(i);
      }
   }
   
public:

   MEM_METHODS(match)
   
   inline size_t size(void) const { return matches.size(); }
   
   inline bool has_any_exact(void) const { return any_exact; }
   
   inline void get_match_stack(match_stack& stk) const {
		for(int i(matches.size()-1); i >= 0; --i)
         stk.push(matches[i]);
   }
   
   inline void match_int(const field_num& num, const int_val val) {
      assert(num < matches.size());
      matches[num].exact = true;
      matches[num].ty = TYPE_INT;
		SET_FIELD_INT(matches[num].field, val);
      any_exact = true;
   }
   
   inline void match_float(const field_num& num, const float_val val) {
      assert(num < matches.size());
      matches[num].exact = true;
      matches[num].ty = TYPE_FLOAT;
      SET_FIELD_FLOAT(matches[num].field, val);
      any_exact = true;
   }
   
   inline void match_node(const field_num& num, const node_val val) {
      assert(num < matches.size());
      matches[num].exact = true;
      matches[num].ty = TYPE_NODE;
      SET_FIELD_NODE(matches[num].field, val);
      any_exact = true;
   }
   
   inline void print(std::ostream& cout) const {
      for(size_t i(0); i < matches.size(); ++i) {
         if(matches[i].exact) {
            switch(matches[i].ty->get_type()) {
               case FIELD_INT: cout << (int)i << " -> " << FIELD_INT(matches[i].field) << std::endl; break;
               case FIELD_FLOAT: cout << (int)i << " -> " << FIELD_FLOAT(matches[i].field) << std::endl; break;
               case FIELD_NODE: cout << (int)i << " -> " << FIELD_NODE(matches[i].field) << std::endl; break;
               default: assert(false);
            }
         }
      }
   }
   
   explicit match(const predicate *pred):
      any_exact(false)
   {
      matches.resize(pred->num_fields());
      set_any_all(pred);
   }
};
   
}

#endif
