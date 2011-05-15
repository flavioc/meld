
#ifndef CONF_HPP
#define CONF_HPP

#include <cstdlib>

//#define ALLOCATOR_ASSERT 1
//#define DEBUG_SAFRAS 1
//#define DEBUG_REMOTE 1
//#define DEBUG_ACTIVE 1
// #define ASSERT_THREADS 1
//#define DEBUG_SERIALIZATION_TIME 1

/* use memory pools for each thread or not */
const bool USE_ALLOCATOR = true;

/* delay to check if messages were sent (in order to delete them) */
const size_t MPI_ROUND_TRIP_UPDATE(200);
/* delay to transmit all messages */
const size_t MPI_ROUND_TRIP_SEND(1000);
/* default delay to fetch messages */
const size_t MPI_DEFAULT_ROUND_TRIP_FETCH(500);
/* min delay round trip to fetch new messages in MPI */
const size_t MPI_MIN_ROUND_TRIP_FETCH(50);
/* delay to decrease the round trip to fetch new messages if a message is found */
const size_t MPI_DECREASE_ROUND_TRIP_FETCH(100);
/* delay to increase the round trip to fetch new messages if messages are not found */
const size_t MPI_INCREASE_ROUND_TRIP_FETCH(1000);
/* max delay round trip to fetch new messages in MPI */
const size_t MPI_MAX_ROUND_TRIP_FETCH(100000);
/* size of buffer to send MPI messages */
const size_t MPI_BUF_SIZE(4096); /* this is the MPI max for Recv */
/* number of threads to ask for nodes */
const size_t MAX_ASK_STEAL(3);
/* number of nodes to send to other threads */
const size_t MAX_SEND_PER_TIME(10);
/* if a message buffer reaches this size, the buffer is sent */
const size_t MPI_BUFFER_THRESHOLD(MPI_BUF_SIZE);
/* threshold to use in global/threads_static to flush work to other threads */
const size_t THREADS_GLOBAL_WORK_FLUSH(20);

#endif
