
#ifndef MEM_STAT_HPP
#define MEM_STAT_HPP

#include <cstdlib>

namespace mem
{

#ifdef MEMORY_STATISTICS

void register_allocation(void *, const size_t, const size_t);
void register_deallocation(void *, const size_t, const size_t);

size_t get_memory_in_use(void);

void register_malloc(const size_t);

size_t get_num_mallocs(void);
size_t get_total_memory(void);

#else

#define register_allocation(P, CNT, SIZE) /* do nothing */
#define register_deallocation(P, CNT, SIZE) /* do nothing */
#define register_malloc(size) /* do nothing */

#endif

}

#endif
