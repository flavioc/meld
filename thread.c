
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "api.h"
#include "node.h"
#include "core.h"
#include "utils.h"
#include "thread.h"
#include "stats.h"

#define MAX_FINISH_FUNCTIONS 10

Thread **threads = NULL;
int NUM_THREADS = 0;
static volatile int idle_threads = 0;
static int finish_functions_count = 0;
static ProgramFinishFunction finish_functions[MAX_FINISH_FUNCTIONS];
static pthread_mutex_t idle_mutex = PTHREAD_MUTEX_INITIALIZER;

void
thread_init(Thread *th)
{
  th->head_queue = th->tail_queue = NULL;
	th->state = THREAD_ACTIVE;
	th->gc_nodes = block_list_create(8);

  pthread_mutex_init(&th->th_mutex, NULL);
	pthread_mutex_init(&th->work_mutex, NULL);

#ifndef USE_WORKSTEALING
  pthread_cond_init(&th->th_cond, NULL);
	th->jobs = 0;
#endif
}

void
vm_add_finish_function(ProgramFinishFunction fn)
{
  finish_functions[finish_functions_count++] = fn;
  assert(finish_functions_count <= MAX_FINISH_FUNCTIONS);
}

static inline void
thread_add_node(Thread *th, Node *node)
{
  node->th = th;
	node->next = th->head_queue;
  node->prev = NULL;
	if(th->head_queue)
		th->head_queue->prev = node;
	th->head_queue = node;
	if(!th->tail_queue)
    th->tail_queue = node;
}

void
thread_add_initial_node(Thread *th, Node *node)
{
	thread_add_node(th, node);
	block_list_insert(th->gc_nodes, node);
}

void
thread_make_idle(Thread *th)
{
	th->state = THREAD_INACTIVE;

	pthread_mutex_lock(&idle_mutex);

  idle_threads++;
  if(idle_threads == NUM_THREADS) {
    /* run finish functions */
    int i;
    
    for(i = 0; i < finish_functions_count; ++i)
      finish_functions[i]();
    exit(EXIT_SUCCESS);
  }

	pthread_mutex_unlock(&idle_mutex);
}

void
thread_awake(Thread *th)
{
	th->state = THREAD_ACTIVE;

	pthread_mutex_lock(&idle_mutex);
  idle_threads--;
#ifndef USE_WORKSTEALING
	pthread_cond_signal(&th->th_cond);
#endif
	pthread_mutex_unlock(&idle_mutex);
}

void
thread_push_work(Thread *th, Node *node)
{
	pthread_mutex_lock(&th->work_mutex);

	thread_add_node(th, node);

	pthread_mutex_unlock(&th->work_mutex);
}

Node*
thread_pop_work(Thread *th)
{
	pthread_mutex_lock(&th->work_mutex);

	Node *ret = th->head_queue;
	
	if(th->head_queue) {
		th->head_queue = th->head_queue->next;
    if (th->tail_queue == ret)
      th->tail_queue = NULL;
    if (th->head_queue)
      th->head_queue->prev = NULL;
      
    ret->next = NULL;
    ret->state = NODE_ACTIVE;
  }

  pthread_mutex_unlock(&th->work_mutex);

	return ret;
}

static Node*
steal_work_queue(Thread *th, Thread *stealer)
{
  pthread_mutex_lock(&th->work_mutex);
  
  Node *ret = NULL;
  
  if(th->tail_queue != NULL) {
    ret = th->tail_queue;
    if (th->tail_queue == th->head_queue)
      th->head_queue = th->tail_queue = NULL;
    else {
      th->tail_queue = th->tail_queue->prev;
      th->tail_queue->next = NULL;
    }
  }

	if(ret)
		ret->th = stealer;

  pthread_mutex_unlock(&th->work_mutex);

  return ret;
}

void
thread_print_work(Thread *th)
{
  printf("Work queue for thread %d ", th->id);
  
  Node *node = th->head_queue;
  while(node) {
    
    printf(NODE_FORMAT " ", node->order);
    
    node = node->next;
  }
  printf("\n");
}

static inline int
move_random_index(int index, int self)
{
  if (index == self)
    return (index + 1) % NUM_THREADS;
  else
    return index;
}

static inline Thread*
select_busy_thread(Thread *self)
{
  int attempts = STEAL_ATTEMPTS;
  int myid = self->id;
  
  while (attempts-- > 0) {
    int idx = move_random_index(random_int(NUM_THREADS), myid);
    
    Thread *selected = threads[idx];
    if(selected->state == THREAD_ACTIVE)
      return selected;
  }
  
  return NULL;
}

Node*
thread_steal_job(Thread *self)
{
  if (NUM_THREADS == 1)
    return NULL;
    
  Thread *selected = select_busy_thread(self);
  
  if(selected == NULL)
    return NULL;
  
  Node *node = steal_work_queue(selected, self);

	/* we can lose our node */
  
	return node;
}
