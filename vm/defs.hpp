
#ifndef DEFS_HPP
#define DEFS_HPP

#include <cstring>
#include <stdint.h>

namespace vm {

typedef unsigned short field_num;
typedef uint32_t uint_val;
typedef int32_t int_val;
typedef double float_val;
typedef uint64_t ptr_val;
typedef uint32_t node_val;
typedef bool bool_val;
typedef unsigned char predicate_id;
typedef unsigned short process_id;
typedef short derivation_count;
typedef uint64_t ref_count;
typedef unsigned char byte_code_el;
typedef byte_code_el* byte_code;
typedef uint32_t code_size_t;
typedef code_size_t code_offset_t;
typedef byte_code pcounter;
typedef size_t strat_level;
typedef size_t argument_id;
typedef uint_val const_id;
typedef uint32_t rule_id;
typedef uint32_t depth_t;

static const ptr_val null_ptr_val = 0;

typedef union {
   bool_val bool_field;
	int_val int_field;
	float_val float_field;
	node_val node_field;
	ptr_val ptr_field;
} tuple_field;

#define FIELD_BOOL(F) ((F).bool_field)
#define FIELD_INT(F) ((F).int_field)
#define FIELD_FLOAT(F) ((F).float_field)
#define FIELD_NODE(F) ((F).node_field)
#define FIELD_PTR(F) ((F).ptr_field)
#define FIELD_CONS(F) ((runtime::cons*)FIELD_PTR(F))
#define FIELD_STRING(F) ((runtime::rstring*)FIELD_PTR(F))
#define FIELD_PCOUNTER(F) ((pcounter)FIELD_PTR(F))
#define FIELD_STRUCT(F) ((runtime::struct1*)FIELD_PTR(F))

#define SET_FIELD_BOOL(F, B) ((F).bool_field = (vm::bool_val)(B))
#define SET_FIELD_INT(F, I) ((F).int_field = (vm::int_val)(I))
#define SET_FIELD_FLOAT(F, L) ((F).float_field = (vm::float_val)(L))
#define SET_FIELD_NODE(F, N) ((F).node_field = (vm::node_val)(N))
#define SET_FIELD_PTR(F, P) ((F).ptr_field = (vm::ptr_val)(P))
#define SET_FIELD_CONS(F, C) (SET_FIELD_PTR(F, C))
#define SET_FIELD_STRUCT(F, S) (SET_FIELD_PTR(F, S))
#define SET_FIELD_STRING(F, S) (SET_FIELD_PTR(F, S))

}

#endif
