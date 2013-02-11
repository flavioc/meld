
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
#define DECLARE_INT(NAME) const int_val NAME(__ ## NAME.int_field)
#define DECLARE_NODE(NAME) const node_val NAME(__ ## NAME.node_field)
#define DECLARE_FLOAT(NAME) const float_val NAME(__ ## NAME.float_field)
#define DECLARE_INT_LIST(NAME) const int_list *NAME((int_list *)__ ## NAME.ptr_field)
#define DECLARE_FLOAT_LIST(NAME) const float_list *NAME((float_list *)__ ## NAME.ptr_field)
#define DECLARE_NODE_LIST(NAME) const node_list *NAME((node_list *)__ ## NAME.ptr_field)
#define DECLARE_STRING(NAME) const rstring::ptr NAME((rstring::ptr)__ ## NAME.ptr_field)
#define RETURN_PTR(X) { argument _ret; _ret.ptr_field = (ptr_val)(X); return _ret; }
#define RETURN_INT(X) { argument _ret; _ret.int_field = X; return _ret; }
#define RETURN_FLOAT(X) { argument _ret; _ret.float_field = X; return _ret; }
#define RETURN_NODE(X) { argument _ret; _ret.node_field = X; return _ret; }
#define RETURN_INT_LIST(X) RETURN_PTR(X)
#define RETURN_FLOAT_LIST(X) RETURN_PTR(X)
#define RETURN_NODE_LIST(X) RETURN_PTR(X)
#define RETURN_STRING(X) RETURN_PTR(X)

class external_function
{
private:
   
   const size_t num_args;
   external_function_ptr ptr;
   field_type ret;
   field_type *spec;
   
public:
   
   inline size_t get_num_args(void) const { return num_args; }
   
   inline field_type get_return_type(void) const { return ret; }
   inline field_type get_arg_type(const size_t i) const { return spec[i]; }
   
   inline external_function_ptr get_fun_ptr(void) const { return ptr; }
   
   void set_arg_type(const size_t, const field_type);
   
   explicit external_function(external_function_ptr, const size_t, const field_type);
   
   ~external_function(void);
};
   
external_function_id register_external_function(external_function *);
external_function* lookup_external_function(const external_function_id);

}

#endif
