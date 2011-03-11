
#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>

#include "api.h"
#include "list.h"
#include "node.h"

#define STEAL_ATTEMPTS 4

typedef enum {
	THREAD_ACTIVE,
	THREAD_INACTIVE
} thread_state;

typedef struct _Thread Thread;
struct _Thread {
  int id;
	BlockList *gc_nodes;

	Node *head_queue;
  Node *tail_queue;

	Node *current_node;

	void *reverse_list;

#ifndef USE_WORKSTEALING
	big_counter jobs;
#endif

	thread_state state;

  pthread_cond_t th_cond;
  pthread_mutex_t th_mutex;
	pthread_mutex_t work_mutex;
};

void thread_add_initial_node(Thread *th, Node *node);

#define ITERATE_INITIAL_QUEUE(thread, name) \
		Node *name = thread->head_queue; \
		for(; name != NULL;  \
				name = name->next)

extern Thread **threads;
extern int NUM_THREADS;

typedef void (*ProgramFinishFunction)(void);

void thread_init(Thread *th);
void vm_add_finish_function(ProgramFinishFunction fn);

Thread *thread_awake_one(void);
void thread_make_idle(Thread *th);
void thread_awake(Thread *th);

void thread_push_work(Thread *th, Node *n);
Node* thread_pop_work(Thread *th);

static inline int
thread_has_work(Thread *th)
{
  return th->head_queue != NULL;
}

Node* thread_steal_job(Thread *self);

void thread_print_work(Thread *th);

#endif /* THREAD_H */

