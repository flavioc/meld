
#ifndef DEFS_HPP
#define DEFS_HPP

#include <cstring>
#include <stdint.h>

namespace vm {

typedef unsigned short field_num;
typedef uint32_t uint_val;
typedef int32_t int_val;
typedef float float_val;
typedef void* addr_val;
typedef bool bool_val;
typedef unsigned char predicate_id;
typedef short ref_count;
typedef unsigned char byte_code_el;
typedef byte_code_el* byte_code;
typedef byte_code pcounter;

}

#endif