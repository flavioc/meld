
#ifndef VM_EXTERNAL_HPP
#define VM_EXTERNAL_HPP

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "utils/types.hpp"

namespace vm
{
   
const size_t EXTERNAL_ARG_LIMIT(100);

typedef size_t external_function_id;

typedef tuple_field argument;
typedef argument (*external_function_ptr0)();
typedef argument (*external_function_ptr1)(argument);
typedef argument (*external_function_ptr2)(argument,argument);
typedef argument (*external_function_ptr3)(argument, argument, argument);
typedef external_function_ptr0 external_function_ptr;

#define EXTERNAL_ARG(NAME) const argument __ ## NAME
#define DECLARE_INT(NAME) const int_val NAME(FIELD_INT(__ ## NAME))
#define DECLARE_NODE(NAME) const node_val NAME(FIELD_NODE(__ ## NAME))
#define DECLARE_FLOAT(NAME) const float_val NAME(FIELD_FLOAT(__ ## NAME))
#define DECLARE_LIST(NAME) const runtime::cons *NAME(FIELD_CONS(__ ## NAME))
#define DECLARE_STRING(NAME) const rstring::ptr NAME(FIELD_STRING(__ ## NAME))
#define DECLARE_STRUCT(NAME) const runtime::struct1 *NAME(FIELD_STRUCT(__ ## NAME))
#define RETURN_PTR(X) { argument _ret; SET_FIELD_PTR(_ret, X); return _ret; }
#define RETURN_INT(X) { argument _ret; SET_FIELD_INT(_ret, X); return _ret; }
#define RETURN_FLOAT(X) { argument _ret; SET_FIELD_FLOAT(_ret, X); return _ret; }
#define RETURN_NODE(X) { argument _ret; SET_FIELD_NODE(_ret, X); return _ret; }
#define RETURN_LIST(X) RETURN_PTR(X)
#define RETURN_STRING(X) RETURN_PTR(X)
#define RETURN_STRUCT(X) RETURN_PTR(X)

class external_function
{
private:
   
   const size_t num_args;
   external_function_ptr ptr;
   type *ret;
   type **spec;
   
public:
   
   inline size_t get_num_args(void) const { return num_args; }
   
   inline type* get_return_type(void) const { return ret; }
   inline type* get_arg_type(const size_t i) const { return spec[i]; }
   
   inline external_function_ptr get_fun_ptr(void) const { return ptr; }
   
   void set_arg_type(const size_t, type*);
   
   explicit external_function(external_function_ptr, const size_t, type*);
   
   ~external_function(void);
};
   
external_function_id register_external_function(external_function *);
external_function* lookup_external_function(const external_function_id);

}

#endif
