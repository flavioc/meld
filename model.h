#ifndef __MODEL_H_
#define __MODEL_H_

#include "thread.h"
#include "stats.h"

extern inline Thread* thread_self(void);

static inline Node*
thread_node(void)
{
	return thread_self()->current_node;
}

extern void node_assert(Node *, tuple_t);

/* allocation for tuples */
#define ALLOC_TUPLE(x) meld_malloc(x)
#define FREE_TUPLE(x) meld_free(x)

#define TUPLES (thread_node()->tuples)
#define OLDTUPLES (thread_node()->oldTuples)
#define PERSISTENT (thread_node()->persistent)
#define PROVED (thread_node()->proved)
#define PUSH_NEW_TUPLE(tuple) (node_assert(thread_node(), tuple))
#define TERMINATE_CURRENT() (node_terminate(thread_node()))

#define EVAL_HOST (&thread_self()->current_node)

#endif
