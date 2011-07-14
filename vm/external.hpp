
#ifndef VM_EXTERNAL_HPP
#define VM_EXTERNAL_HPP

#include "vm/types.hpp"
#include "vm/defs.hpp"
#include "utils/types.hpp"

namespace vm
{
   
const size_t EXTERNAL_ARG_LIMIT(100);

typedef utils::byte external_function_id;

typedef all_val argument;
typedef argument (*external_function_ptr0)();
typedef argument (*external_function_ptr1)(argument);
typedef argument (*external_function_ptr2)(argument,argument);
typedef argument (*external_function_ptr3)(argument, argument, argument);
typedef external_function_ptr0 external_function_ptr;

#define FROM_ARG(X, TYPE) (*(TYPE*)&(X))
#define TO_ARG(X,ARG) (memcpy(&(ARG), &(X), sizeof(X)))
#define EXTERNAL_ARG(NAME) const argument __ ## NAME
#define DECLARE_ARG(NAME,TYPE) const TYPE NAME(FROM_ARG(__ ## NAME, TYPE)) 
#define RETURN_ARG(X) { argument _ret; TO_ARG(X,_ret); return _ret; }

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
