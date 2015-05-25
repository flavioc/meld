
#ifndef VM_MATCH_HPP
#define VM_MATCH_HPP

#include <stack>
#include <ostream>

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "mem/allocator.hpp"
#include "vm/predicate.hpp"
#include "utils/stack.hpp"
#include "vm/types.hpp"
#include "vm/instr.hpp"

namespace vm {

enum variable_match_type {
   MATCH_REG = 0,
   MATCH_FIELD = 1,
   MATCH_HOST = 2
};

struct match_field {
   bool exact;
   type *ty;
   tuple_field field;
};

struct variable_match_template {
   match_field *match;
   variable_match_type type;
   reg_num reg{0};
   field_num field{0};
};

typedef utils::stack<match_field> match_stack;

struct list_match {
   match_field head;
   match_field tail;

   inline void init(list_type *lt) {
      head.exact = false;
      head.ty = lt->get_subtype();
      tail.exact = false;
      tail.ty = lt;
   }

   inline void destroy(void) {
      if (head.exact) {
         if (head.ty->get_type() == FIELD_LIST) {
            if (head.field.ptr_field > 1) {
               list_match *m(FIELD_LIST_MATCH(head.field));
               m->destroy();
               mem::allocator<list_match>().deallocate(m, 1);
            }
         }
      }

      if (tail.exact) {
         if (tail.ty->get_type() == FIELD_LIST) {
            if (tail.field.ptr_field > 1) {
               list_match *m(FIELD_LIST_MATCH(tail.field));
               m->destroy();
               mem::allocator<list_match>().deallocate(m, 1);
            }
         }
      }
   }
};

inline void print_match_field(const match_field &m, std::ostream &cout) {
   if (!m.exact) {
      cout << "Not exact " << m.ty->string();
      return;
   }
   switch (m.ty->get_type()) {
      case FIELD_INT:
         cout << "int -> " << FIELD_INT(m.field);
         break;
      case FIELD_FLOAT:
         cout << "float -> " << FIELD_FLOAT(m.field);
         break;
      case FIELD_NODE:
         cout << "node -> @" << FIELD_NODE(m.field);
         break;
      default:
         cout << " print not implemented for this type";
   }
}

typedef struct _match {
   bool any_exact;
   size_t matches_size;
   size_t var_size;

   inline size_t size(void) const { return matches_size; }

   static inline size_t mem_size(const predicate *pred, const size_t var_size) {
      return sizeof(struct _match) + pred->num_fields() * sizeof(match_field) +
             var_size * sizeof(variable_match_template);
   }

   inline size_t mem_size(void) const {
      return sizeof(struct _match) + matches_size * sizeof(match_field) +
             var_size * sizeof(variable_match_template);
   }

   inline const match_field *get_matches(void) const {
      return (match_field *)(this + 1);
   }
   inline match_field *get_matches(void) { return (match_field *)(this + 1); }
   inline const variable_match_template *get_vmt(void) const {
      return (variable_match_template *)((utils::byte *)(this + 1) +
                                         sizeof(match_field) * matches_size);
   }
   inline variable_match_template *get_vmt(void) {
      return (variable_match_template *)((utils::byte *)(this + 1) +
                                         sizeof(match_field) * matches_size);
   }

   inline void set_any_all(const predicate *pred) {
      for (size_t i(0); i < matches_size; ++i) {
         get_matches()[i].exact = false;
         get_matches()[i].ty = pred->get_field_type(i);
      }
   }

   inline bool has_match(const field_num &num) const {
      return get_matches()[num].exact;
   }

   inline void unset_match(const field_num &num) {
      get_matches()[num].exact = false;
   }

   inline void restore_match(const field_num &num) {
      get_matches()[num].exact = true;
   }

   inline match_field get_match(const field_num &num) const {
      return get_matches()[num];
   }
   inline match_field *get_update_match(const field_num &num) {
      return get_matches() + num;
   }

   inline void get_match_stack(match_stack &stk) const {
      for (int i(matches_size - 1); i >= 0; --i) stk.push(get_match(i));
   }

   inline void add_variable_match(const variable_match_template &vmt,
                                  const size_t i) {
      assert(i < var_size);
      get_vmt()[i] = vmt;
   }

   inline variable_match_template &get_variable_match(const size_t i) {
      return get_vmt()[i];
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

   inline void print(std::ostream &cout) const {
      for (size_t i(0); i < matches_size; ++i) {
         cout << (int)i << " ";
         print_match_field(get_match(i), cout);
         cout << std::endl;
      }
   }

   inline void init(const predicate *pred, const size_t _var_size) {
      any_exact = false;
      var_size = _var_size;
      matches_size = pred->num_fields();
      set_any_all(pred);
   }

   inline void destroy(void) {
      for (size_t i(0); i < matches_size; ++i) {
         match_field m(get_match(i));
         if (m.ty->get_type() == FIELD_LIST && m.exact) {
            if (m.field.ptr_field > 1) {
               list_match *lm(FIELD_LIST_MATCH(m.field));
               lm->destroy();
               mem::allocator<list_match>().deallocate(lm, 1);
            }
         }
      }
   }
} match;
}

#endif
