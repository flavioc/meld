
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
#include "vm/instr.hpp"

namespace vm
{

struct match_field {
   bool exact;
   type *ty;
	tuple_field field;
};

struct variable_match_template {
   reg_num reg;
   field_num field;
   match_field *match;
};

typedef utils::stack<match_field> match_stack;

class list_match: public mem::base
{
   public:

      match_field head;
      match_field tail;

      explicit list_match(list_type *lt) {
         head.exact = false;
         head.ty = lt->get_subtype();
         tail.exact = false;
         tail.ty = lt;
      }
};

inline void
print_match_field(const match_field& m, std::ostream& cout)
{
   if(!m.exact) {
      cout << "Not exact " << m.ty->string();
      return;
   }
   switch(m.ty->get_type()) {
      case FIELD_INT: cout << "int -> " << FIELD_INT(m.field); break;
      case FIELD_FLOAT: cout << "float -> " << FIELD_FLOAT(m.field); break;
      case FIELD_NODE: cout << "node -> @" << FIELD_NODE(m.field); break;
      default: cout << " print not implemented for this type";
   }
}

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

   std::vector<variable_match_template> variable_matches;

   MEM_METHODS(match)
   
   inline size_t size(void) const { return matches.size(); }
   
   inline bool has_any_exact(void) const { return any_exact; }

   inline bool has_match(const field_num& num) const
   {
      return matches[num].exact;
   }

   inline match_field get_match(const field_num& num) const { return matches[num]; }
   inline match_field *get_update_match(const field_num& num) { return &matches[num]; }
   
   inline void get_match_stack(match_stack& stk) const {
		for(int i(matches.size()-1); i >= 0; --i)
         stk.push(matches[i]);
   }

   inline void add_variable_match(const variable_match_template& vmt) {
      variable_matches.push_back(vmt);
   }
   
   inline void match_int(match_field *m, const int_val val) {
      m->exact = true;
		SET_FIELD_INT(m->field, val);
      any_exact = true;
   }
   
   inline void match_float(match_field *m, const float_val val) {
      m->exact = true;
      SET_FIELD_FLOAT(m->field, val);
      any_exact = true;
   }
   
   inline void match_node(match_field *m, const node_val val) {
      m->exact = true;
      SET_FIELD_NODE(m->field, val);
      any_exact = true;
   }

   inline void match_non_nil(match_field *m) {
      m->exact = true;
      SET_FIELD_PTR(m->field, 1);
      any_exact = true;
   }

   inline void match_nil(match_field *m) {
      m->exact = true;
      SET_FIELD_PTR(m->field, 0);
      any_exact = true;
   }

   inline void match_list(match_field *m, list_match *lm, type *t) {
      m->exact = true;
      m->ty = t;
      SET_FIELD_LIST_MATCH(m->field, lm);
      any_exact = true;
   }
   
   inline void print(std::ostream& cout) const {
      for(size_t i(0); i < matches.size(); ++i) {
         cout << (int)i << " ";
         print_match_field(matches[i], cout);
         cout << std::endl;
      }
   }
   
   explicit match(const predicate *pred):
      any_exact(false)
   {
      matches.resize(pred->num_fields());
      set_any_all(pred);
   }

   ~match(void)
   {
      for(size_t i(0); i < matches.size(); ++i) {
         if(matches[i].ty->get_type() == FIELD_LIST && matches[i].exact) {
            if(matches[i].field.ptr_field > 1) {
               list_match *m(FIELD_LIST_MATCH(matches[i].field));
               delete m;
            }
         }
      }
   }
};
   
}

#endif
