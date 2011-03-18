
#ifndef API_H
#define API_H

#include <stdint.h>
#include "config.h"
#include "defs.h"

typedef int32_t meld_int;
typedef float meld_float;
typedef meld_int meld_bool;
typedef short tuple_type;
typedef void* tuple_t;
typedef void* anything;
typedef unsigned char aggregate_t; /* for aggregate fields */
typedef unsigned char aggregate_type; /* aggregate type */
typedef unsigned char aggregate_field; /* aggregate field number */
typedef anything field_t;
typedef int ref_count;
typedef unsigned char *pcounter;
typedef unsigned long int big_counter;
typedef anything meld_value;
typedef Register meld_return;

#define NODE_FORMAT "%lu"

#define MELD_INT(x)   (*(meld_int *)(x))
#define MELD_FLOAT(x) (*(meld_float *)(x))
#define MELD_BOOL(x)	(*(meld_bool *)(x))
#define MELD_NODE(x)  (*(Node **)(x))
#define MELD_NODE_ID(x) (*(NodeID *)(x))
#define MELD_SET(x) (*(Set **)(x))
#define MELD_LIST(x)  (*(List **)(x))
#define MELD_PTR(x)	  (*(void **)(x))

#define MELD_RETURN_VAL _meld_return
#define MELD_RETURN_PARAM meld_return MELD_RETURN_VAL
#define MELD_ARG1 _meld_arg1
#define MELD_ARG2 _meld_arg2
#define MELD_ARG3 _meld_arg3
#define MELD_ARG4 _meld_arg4
#define MELD_ARG5 _meld_arg5

#define MELD_RETURN_FLOAT(float_val) \
	MELD_FLOAT(MELD_RETURN_VAL) = float_val; \
	return

#define MELD_RETURN_INT(int_val) \
	MELD_INT(MELD_RETURN_VAL) = int_val;	\
	return

#define MELD_RETURN_LIST(list) \
	MELD_LIST(MELD_RETURN_VAL) = list;	\
	return

#define MELD_RETURN_SET(set) \
	MELD_SET(MELD_RETURN_VAL) = set;	\
	return

#define MELD_RETURN_NOTHING() MELD_RETURN_INT(0)

/* if you want to declare a Meld function without using
	 the default arguments names, you can do this:

	 void function_name(MELD_RETURN_PARAM, meld_value my_arg, ...)
	 {
	 		( ... function code ... )
		}
*/
#define MELD_FUNCTION0(name) void name(MELD_RETURN_PARAM)
#define MELD_FUNCTION1(name) void name(MELD_RETURN_PARAM, \
		meld_value MELD_ARG1)
#define MELD_FUNCTION2(name) void name(MELD_RETURN_PARAM, \
		meld_value MELD_ARG1, meld_value MELD_ARG2)
#define MELD_FUNCTION3(name) void name(MELD_RETURN_PARAM, \
		meld_value MELD_ARG1, meld_value MELD_ARG2,						\
		meld_value MELD_ARG3)
#define MELD_FUNCTION4(name) void name(MELD_RETURN_PARAM, \
		meld_value MELD_ARG1, meld_value MELD_ARG3,						\
		meld_value MELD_ARG4)
#define MELD_FUNCTION5(name) void name(MELD_RETURN_PARAM, \
		meld_value MELD_ARG1, meld_value MELD_ARG3,						\
		meld_value MELD_ARG4, meld_value MELD_ARG5)

#define MELD_DECLARE_FUNCTION0(name) void name(meld_return)
#define MELD_DECLARE_FUNCTION1(name) void name(meld_return, meld_value)
#define MELD_DECLARE_FUNCTION2(name) void name(meld_return, meld_value, \
									meld_value)
#define MELD_DECLARE_FUNCTION3(name) void name(meld_return, meld_value, \
									meld_value, meld_value)
#define MELD_DECLARE_FUNCTION4(name) void name(meld_return, meld_value, \
									meld_value, meld_value, meld_value)
#define MELD_DECLARE_FUNCTION5(name) void name(meld_return, meld_value, \
									meld_value, meld_value, meld_value, meld_value)

#endif
