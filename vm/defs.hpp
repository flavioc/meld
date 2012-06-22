
#ifndef DEFS_HPP
#define DEFS_HPP

#include <cstring>
#include <stdint.h>

namespace vm {

typedef unsigned short field_num;
typedef uint32_t uint_val;
typedef int32_t int_val;
typedef float float_val;
typedef unsigned long int ptr_val;
typedef unsigned long int all_val;
typedef uint32_t node_val;
typedef uint32_t worker_val;
typedef bool bool_val;
typedef unsigned char predicate_id;
typedef unsigned short process_id;
typedef short ref_count;
typedef unsigned char byte_code_el;
typedef byte_code_el* byte_code;
typedef uint32_t code_size_t;
typedef code_size_t code_offset_t;
typedef byte_code pcounter;
typedef size_t strat_level;
typedef size_t argument_id;
typedef uint_val const_id;

static const ptr_val null_ptr_val = 0;

}

#endif