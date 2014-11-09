
#include <limits>

#include "vm/all.hpp"
#include "vm/program.hpp"

namespace vm
{

inline bool
higher_priority(const priority_t p1, const priority_t p2)
{
   if(theProgram->is_priority_desc())
      return p1 > p2;
   else
      return p1 < p2;
}

inline bool
no_priority_value(void)
{
   if(theProgram->is_priority_desc())
      return std::numeric_limits<priority_t>::min();
   else
      return std::numeric_limits<priority_t>::max();
}

}
