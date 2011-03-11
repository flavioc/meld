
#ifndef CONFIG_H
#define CONFIG_H

/* comment if your pthreads implementation supports barriers */
#define WITH_BARRIER

#ifdef CYGWIN
#ifndef WITH_BARRIER
#define WITH_BARRIER
#endif
#endif

/* comment if you use Mac OS X */
#define MACOSX 1

/* uncomment to disable garbage collector */
//#define DO_GC 1

/* comment to disable statistics */
#define STATISTICS 1

/* COMPLEX_PARTITION uses a partition scheme where the nodes
	 are partitioned among threads by selecting the next available
	 nodes. For simple partitioning the nodes are partitioned without
	 any respect to the closeness of the nodes */
#define COMPLEX_PARTITION 1

/* use google's tcmalloc */
// #define USE_TCMALLOC 1

/* uncomment to use a simpler work sharing scheme */
//#define USE_WORKSTEALING 1

#define PARALLEL_MACHINE 1 /* compile for parallel vm */

#ifdef USE_TCMALLOC
#include <google/tcmalloc.h>

#define meld_malloc 	tc_malloc
#define meld_calloc 	tc_calloc
#define meld_free 		tc_free
#define meld_realloc 	tc_realloc

#else

#define meld_malloc malloc
#define meld_calloc calloc
#define meld_free free
#define meld_realloc realloc

#endif

/* activate for a different program */
//#define PAGERANK 1
//#define WALKGRID 1
//#define BELIEF_PROPAGATION 1
//#define SHORTEST_PATH 1
//#define CONNECTIVITY 1
//#define SET_GC_TEST 1
#define MELD_OTHER 1

#endif /* CONFIG_H */

