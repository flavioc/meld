
#ifndef MEM_STAT_HPP
#define MEM_STAT_HPP

#include "conf.hpp"

namespace mem
{

#ifdef MEMORY_STATISTICS

void register_allocation(const size_t, const size_t);
void register_deallocation(const size_t, const size_t);

size_t get_memory_in_use(void);

void register_malloc(void);

size_t get_num_mallocs(void);

#else

#define register_allocation(CNT, SIZE) /* do nothing */
#define register_deallocation(CNT, SIZE) /* do nothing */
#define register_malloc() /* do nothing */

#endif

}

#endif
