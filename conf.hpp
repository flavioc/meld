
#ifndef CONF_HPP
#define CONF_HPP

#include <cstdlib>

// activate this to collect statistics on memory use
//#define MEMORY_STATISTICS 1

// gather statistics about the core VM execution
// #define CORE_STATISTICS 1

// activate instrumentation code
//#define INSTRUMENTATION 1

// gather fact statistics
// #define FACT_STATISTICS 1

// use memory pools for each thread or not
const bool USE_ALLOCATOR = true;

// when this is activated, the virtual machine will update every reference to abstract node addresses
// in the bytecode to concrete node addresses in memory
// for multithreaded meld this will improve speed since there's one less lookup
#define USE_REAL_NODES 1

// faster multithreading by allowing threads to index on other thread's nodes
#define FASTER_INDEXING 1

// activate dynamic indexing of linear facts
#define DYNAMIC_INDEXING 1

#endif

