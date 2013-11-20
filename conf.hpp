
#ifndef CONF_HPP
#define CONF_HPP

#include <cstdlib>

// when this is active we active a more strict debugging of threads
// #define ASSERT_THREADS 1

// when this is active the allocator checks for allocate/dealloc correctness
// #define ALLOCATOR_ASSERT 1

// activate this to collect statistics on memory use
//#define MEMORY_STATISTICS 1

// activate special code for testing trie matching
// #define TRIE_MATCHING_ASSERT 1

// gather statistics about the core VM execution
//#define CORE_STATISTICS 1

// activate instrumentation code
// #define INSTRUMENTATION 1

// use simulator
//#define USE_SIM

// use memory pools for each thread or not
const bool USE_ALLOCATOR = true;

#endif
