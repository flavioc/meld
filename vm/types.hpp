#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstring>
#include <string>
#include <stdexcept>

namespace vm {
   
enum field_type {
   FIELD_INT = 0x0,
   FIELD_FLOAT = 0x1,
   FIELD_NODE = 0x2,
   FIELD_LIST_INT = 0x3,
   FIELD_LIST_FLOAT = 0x4,
   FIELD_LIST_NODE = 0x5,
	FIELD_STRING = 0x9
};

enum aggregate_type {
   AGG_FIRST = 1,
   AGG_MAX_INT = 2,
   AGG_MIN_INT = 3,
   AGG_SUM_INT = 4,
   AGG_MAX_FLOAT = 5,
   AGG_MIN_FLOAT = 6,
   AGG_SUM_FLOAT = 7,
   AGG_SUM_LIST_FLOAT = 11
};

enum aggregate_safeness {
   AGG_LOCALLY_GENERATED = 1,
   AGG_NEIGHBORHOOD = 2,
   AGG_NEIGHBORHOOD_AND_SELF = 3,
   AGG_IMMEDIATE = 4,
   AGG_UNSAFE = 5
};

size_t field_type_size(field_type type);
std::string field_type_string(field_type type);
std::string aggregate_type_string(aggregate_type type);

inline bool aggregate_safeness_uses_neighborhood(const aggregate_safeness safeness)
{
   return safeness == AGG_NEIGHBORHOOD || safeness == AGG_NEIGHBORHOOD_AND_SELF;
}

class type_error : public std::runtime_error {
 public:
    explicit type_error(const std::string& msg) :
         std::runtime_error(msg)
    {}
};

}

#endif
