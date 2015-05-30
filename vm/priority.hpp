
#ifndef VM_PRIORITY_HPP
#define VM_PRIORITY_HPP

#include <limits>

#include "vm/all.hpp"
#include "vm/program.hpp"

namespace vm {

inline bool higher_priority(const priority_t p1, const priority_t p2) {
   if (theProgram->is_priority_desc())
      return p1 > p2;
   else
      return p1 < p2;
}

static inline priority_t max_priority_value0(const bool desc) {
   if (desc)
      return std::numeric_limits<priority_t>::max();
   else
      return std::numeric_limits<priority_t>::lowest();
}

static inline priority_t no_priority_value0(const bool desc) {
   return max_priority_value0(!desc);
}

static inline priority_t no_priority_value() {
   return theProgram->get_no_priority_value();
}

static inline priority_t max_priority_value() {
   return max_priority_value0(theProgram->is_priority_desc());
}

static inline priority_t initial_priority_value0(const bool desc) {
   return max_priority_value0(desc) / 2;
}
}

#endif
