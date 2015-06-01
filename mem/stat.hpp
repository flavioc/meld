
#ifndef MEM_STAT_HPP
#define MEM_STAT_HPP

#include <cstdlib>

namespace mem
{

#ifdef MEMORY_STATISTICS

void register_allocation(void *, const size_t, const size_t);
void register_deallocation(void *, const size_t, const size_t);
void print_memory_statistics();
void register_malloc(const size_t);
void merge_memory_statistics();

#else

#define register_allocation(P, CNT, SIZE) /* do nothing */
#define register_deallocation(P, CNT, SIZE) /* do nothing */
#define register_malloc(size) /* do nothing */

#endif

}

#endif
