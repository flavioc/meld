
#ifndef CONF_HPP
#define CONF_HPP

#include <cstdlib>

/* when this is active we active a more strict debugging of threads */
// #define ASSERT_THREADS 1

/* when this is active the allocator checks for allocate/dealloc correctness */
// #define ALLOCATOR_ASSERT 1

/* when this is active it activates extra trie checking code */
// #define TRIE_ASSERT 1

/* activate this to collect statistics on memory use */
//#define MEMORY_STATISTICS 1

/* activate tuple matching using the trie structure */
#define TRIE_MATCHING 1

/* activate debug mode */
//#define DEBUG_MODE 1

/* activate special code for testing trie matching */
// #define TRIE_MATCHING_ASSERT 1

/* use old method of counting neighborhood aggregates */
// #define USE_OLD_NEIGHBOR_CHECK 1

/* gather statistics about the core VM execution */
//#define CORE_STATISTICS 1

/* use fact counting for rule engine */
#define USE_RULE_COUNTING 1

/* activate instrumentation code */
// #define INSTRUMENTATION 1

//#define DEBUG_SAFRAS 1
//#define DEBUG_REMOTE 1
//#define DEBUG_ACTIVE 1
//#define DEBUG_SERIALIZATION_TIME 1

/* build hash table of nodes for work stealing schedulers */
#define MARK_OWNED_NODES

/* use ui interface */
// #define USE_UI

/* use simulator */
#define USE_SIM

/* use memory pools for each thread or not */
const bool USE_ALLOCATOR = true;

/* delay to check if messages were sent (in order to delete them) */
const size_t MPI_ROUND_TRIP_UPDATE(200);
/* delay to transmit all messages */
const size_t MPI_DEFAULT_ROUND_TRIP_SEND(250);
/* default delay to fetch messages */
const size_t MPI_DEFAULT_ROUND_TRIP_FETCH(500);
/* min delay round trip to fetch new messages in MPI */
const size_t MPI_MIN_ROUND_TRIP_FETCH(10);
/* delay to decrease the round trip to fetch new messages if a message is found */
const size_t MPI_DECREASE_ROUND_TRIP_FETCH(10);
/* delay to increase the round trip to fetch new messages if messages are not found */
const size_t MPI_INCREASE_ROUND_TRIP_FETCH(10);
/* max delay round trip to fetch new messages in MPI */
const size_t MPI_MAX_ROUND_TRIP_FETCH(100000);
/* size of buffer to send MPI messages */
const size_t MPI_BUF_SIZE(4096); /* this is the MPI max for Recv */
/* if a message buffer reaches this size, the buffer is sent */
const size_t MPI_BUFFER_THRESHOLD(MPI_BUF_SIZE);

/* rounds to delay after we fail to find an active worker */
const size_t DELAY_STEAL_CYCLE(10);
/* factor to compute the number of nodes to send to another worker when stealing */
const size_t STEAL_NODES_FACTOR(1000);

/* threshold to use in global/threads_static to flush work to other threads */
const size_t THREADS_GLOBAL_WORK_FLUSH(20);

#endif
