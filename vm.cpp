#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>
#include <math.h>

#include "thread.h"
#include "utils.h"
#include "node.h"
#include "hash.h"
#include "set_runtime.h"
#include "list_runtime.h"
#include "extern_functions.h"
#include "vm.h"
#include "core.h"
#include "model.h"
#include "stats.h"
#include "allocator.h"

#if 0
#ifdef PAGERANK
#include "progs/pagerank.bb"
#elif WALKGRID
#include "progs/walkgrid.bb"
#elif SHORTEST_PATH
#include "progs/shortest_path.bb"
#elif CONNECTIVITY
#include "progs/connectivity.bb"
#elif BELIEF_PROPAGATION
#include "progs/image_list.bb"
#elif SET_GC_TEST
#include "progs/set_gc_test.bb"
#elif MELD_OTHER
#include "base.bb"
#endif
#endif

static pthread_key_t thread_key = 0;
static pthread_barrier_t init_barrier;
static pthread_barrier_t edge_barrier;
static pthread_barrier_t tuples_barrier;

/* uncomment to debug register use */
#define REGISTER_USE_CHECK 1

/* debug code */

/* print message communication between nodes */
//#define MSG_DEBUG 1
//#define THREAD_DEBUG 1

/* I want to know who am I ... */
Thread* thread_self(void)
{
  return static_cast<Thread*>(pthread_getspecific(thread_key));
}

static inline
int node_queue_is_empty(Node *node)
{
  return queue_is_empty(&node->queueFacts);
}

#ifdef USE_STRAT_QUEUE
static inline
bool node_strat_queue_is_empty(Node *node)
{
  return p_empty(node->newStratTuples);
}
#endif

void
node_do_enqueue(Node *node, tuple_t tuple, int count)
{
  tuple_entry *entry = (tuple_entry *)meld_malloc(sizeof(tuple_entry));
  
  entry->tuple = tuple;
  entry->records.count = count;
  entry->next = NULL;
  
  pthread_mutex_lock(&node->queueMutex);
  queue_push_tuple(&node->queueFacts, entry);
  pthread_mutex_unlock(&node->queueMutex);
}

#ifdef USE_STRAT_QUEUE
void
node_strat_enqueue(Node *node, int round, tuple_t tuple, ref_count isNew)
{
  pthread_mutex_lock(&node->stratMutex);
  p_enqueue(node->newStratTuples, round, tuple, NULL, (record_type)isNew);
  pthread_mutex_unlock(&node->stratMutex);
}
#endif

tuple_t
node_do_dequeue(Node *node, ref_count *count)
{
  tuple_entry *entry;
  
  pthread_mutex_lock(&node->queueMutex);
  entry = (tuple_entry *)queue_pop_tuple(&node->queueFacts);
  pthread_mutex_unlock(&node->queueMutex);
  
  if (entry == NULL)
		return NULL;

  tuple_t tuple = entry->tuple;
  *count = entry->records.count;
  
	meld_free(entry);

	return tuple;
}

static void
node_enqueue(Node *node, tuple_t tuple, ref_count isNew)
{ 
#ifdef USE_STRAT_QUEUE  
  if(TYPE_IS_STRATIFIED(TUPLE_TYPE(tuple)))
    node_strat_enqueue(node, TYPE_STRATIFICATION_ROUND(TUPLE_TYPE(tuple)), tuple, isNew);
  else
#endif
    node_do_enqueue(node, tuple, isNew);

#ifdef USE_WORKSTEALING
  pthread_mutex_lock(&node->stateMutex);
  
  /* update node's state */
	if(node->state == NODE_INACTIVE) {
		/* mark this node as having new tuples */
    node->state = NODE_READY;

    assert(!node->next && !node->prev);
   
    pthread_mutex_unlock(&node->stateMutex);

    Thread *th = node->th;

#ifdef THREAD_DEBUG
		printf(">> Push new work %d\n", th->id);
#endif

		thread_push_work(th, node);
	  
		/* wake up thread if needed */
		pthread_mutex_lock(&th->th_mutex);
		if(th->state == THREAD_INACTIVE) {
#ifdef THREAD_DEBUG
			printf(">> Awaking %d\n", th->id);
#endif
			thread_awake(th);
		}
		pthread_mutex_unlock(&th->th_mutex);
	} else {
#ifdef THREAD_DEBUG
		printf(">> Node ready from thread %d\n", node->th->id);
#endif
    node->state = NODE_READY;
    pthread_mutex_unlock(&node->stateMutex);
  }
#else /* USE_WORKSTEALING */
	Thread *th = node->th;

	__sync_add_and_fetch(&th->jobs, 1);
	if(th->jobs == 1 && th->state == THREAD_INACTIVE) {
		pthread_mutex_lock(&th->th_mutex);
		if(th->state == THREAD_INACTIVE)
			thread_awake(th);
		pthread_mutex_unlock(&th->th_mutex);
	}
#endif

#ifdef ALL_WORK_CHECK
  __increment_total(1);
#endif
}

static inline tuple_t
node_dequeue(Node *node, ref_count *isNew)
{
  tuple_t ret = node_do_dequeue(node, isNew);
  
#ifdef ALL_WORK_CHECK
  __decrement_total(1);
#endif
#ifndef USE_WORKSTEALING
	Thread *th = node->th;
	__sync_sub_and_fetch(&th->jobs, 1);
#endif

  return ret;
}

void
node_assert(Node *node, tuple_t tuple)
{
	node_enqueue(node, tuple, 1);
}

#ifdef USE_STRAT_QUEUE
static inline tuple_t
node_strat_dequeue(Node *node, ref_count *isNew)
{
  pthread_mutex_lock(&node->stratMutex);
  
  tuple_pentry *entry = p_dequeue(node->newStratTuples);

  tuple_t tuple = entry->tuple;
  *isNew = entry->records.count;

  pthread_mutex_unlock(&node->stratMutex);
  
  meld_free(entry);
  
#ifdef ALL_WORK_CHECK
  __decrement_total(1);
#endif

  return tuple;
}
#endif /* USE_STRAT_QUEUE */

tuple_type
type_lookup(const char* name)
{
	tuple_type i;
	for(i = 0; i < NUM_TYPES; ++i)
		if(strcmp(TYPE_NAME(i), name) == 0)
			return i;
	
  fprintf(stderr, "Could not find tuple type %s\n", name);
  assert(0);
  exit(EXIT_FAILURE);
	return -1;
}

static inline
void enqueue_init(Node *node)
{
  if(TYPE_INIT == -1)
    return; /* no init for this program */
    
	tuple_t tuple = tuple_alloc(TYPE_INIT);

	SET_TUPLE_FIELD(tuple, 0, &node->id);
	
	node_assert(node, tuple);
}

List*
route_build_direct(Node *p1, Node *p2)
{
  Node *vec[2] = {p1, p2};

	return list_node_from_vector((void**)vec, 2);
}

tuple_t
tuple_build_edge(Node *p1, Node *p2)
{
  tuple_t tuple = tuple_alloc(TYPE_EDGE);

#ifdef MICHAELS_COMPILER
	SET_TUPLE_FIELD(tuple, 0, &p2->id);

  List* route = route_build_direct(p1, p2);

	SET_TUPLE_FIELD(tuple, 1, &route);
#else
	SET_TUPLE_FIELD(tuple, 0, &p2->id);
	(void)p1;
#endif

	return tuple;
}

static inline void
thread_set_node(Thread *th, Node *node)
{
  th->current_node = node;
}

static inline void
node_do_process(Node *node)
{
  ref_count isNew;
  tuple_t tuple;
	Register reg[32];

#ifdef REGISTER_USE_CHECK
  memset(reg, 0, sizeof(reg));
#endif
  
  while (!node_queue_is_empty(node)) {
		isNew = 0;
		tuple = node_dequeue(node, &isNew);
		
#ifdef MSG_DEBUG
		printf(NODE_FORMAT " " , node->order);
		tuple_print(tuple, stdout);
		printf(" %d\n", isNew);
#endif

		tuple_handle(tuple, isNew, reg);

#ifdef DO_GC
		if(allocator_will_collect(th, true))
			allocator_collect(th);
#endif
	}
	
#ifdef USE_STRAT_QUEUE
	while (!node_strat_queue_is_empty(node)) {
    tuple = node_strat_dequeue(node, &isNew);

#ifdef MSG_DEBUG
		printf(NODE_FORMAT " " , node->order);
		tuple_print(tuple, stdout);
		printf(" %d\n", isNew);
#endif

    tuple_handle(tuple, isNew, reg);
	}
#endif
}

static inline void
node_process(Thread *th, Node *node)
{
#ifdef MSG_DEBUG
		printf("== [thread %d] NODE " NODE_FORMAT "(%p) ===========\n", th->id, node->order, node);
#endif

  thread_set_node(th, node);

#ifdef USE_WORKSTEALING
redo_process:
	node_do_process(node);

  pthread_mutex_lock(&node->stateMutex);

  switch(node->state) {
    case NODE_ACTIVE:
      node->state = NODE_INACTIVE; break;
    case NODE_READY:
      /* someone put more work in this node! */
      node->state = NODE_ACTIVE;
      pthread_mutex_unlock(&node->stateMutex);
      goto redo_process;
    default:
      assert(node->state != NODE_INACTIVE);
  }

  pthread_mutex_unlock(&node->stateMutex);
#else
	node_do_process(node);
#endif
}

static inline void
nodes_process(Thread *th)
{
#ifdef USE_WORKSTEALING
  while (thread_has_work(th)) {
		Node *node = thread_pop_work(th);
		
		if(node == NULL) /* it is possible that someone stole my work! */
      continue;

    node_process(th, node);
  }
#else
	ITERATE_INITIAL_QUEUE(th, node) {
		node_process(th, node);
	}
#endif
}

#ifdef USE_WORKSTEALING
static inline void
get_work_or_idle(Thread *th)
{  
	if(!thread_has_work(th)) {
    /* attempt to steal someone else's work ... */
		Node *node = thread_steal_job(th);

		if(!node) {
#ifdef THREAD_DEBUG
			printf("%d: Dormant! %d\n", th->id, totalNew);
#endif

		  /* enter dormant state ... */
			pthread_mutex_lock(&th->th_mutex);
      thread_make_idle(th);
			pthread_mutex_unlock(&th->th_mutex);

			while (!thread_has_work(th) && th->state != THREAD_ACTIVE) {
				node = thread_steal_job(th);
				if(node)
					break;
#ifdef DO_GC
				else
					if(allocator_will_collect(th, false)) {
						allocator_collect(th);
					}
#endif
			}

			pthread_mutex_lock(&th->th_mutex);

			if(th->state == THREAD_INACTIVE) {
				thread_awake(th);
			}

			pthread_mutex_unlock(&th->th_mutex);
		}

		if(node) {
      assert(node->state == NODE_READY);
      assert(node->th == th);
#ifdef THREAD_DEBUG
			printf("%d Stole something!\n", th->id);
#endif
      thread_push_work(th, node);
    }
  }
}
#endif

static inline void
thread_init_tuples(Thread *th)
{
  block_list_iterator it;
  
  ITERATE_INITIAL_QUEUE(th, node) {
		enqueue_init(node);

    /* add initial tuples */
    for(it = block_list_get_iterator(node->first_tuples);
        block_list_iterator_has_next(it);
        it = block_list_iterator_next(it))
      node_assert(node, block_list_iterator_tuple(it));
    
  	node_optimize(node);
	}
}

static inline
void thread_init_nodes(Thread *th)
{
  ITERATE_INITIAL_QUEUE(th, node) {
    node_init(node);
  }
}

static inline
void thread_process_edges(Thread *th)
{
  block_list_iterator it;
	Register reg[32];
  
  ITERATE_INITIAL_QUEUE(th, node) {
    thread_set_node(th, node);
    for(it = block_list_get_iterator(node->neighbors);
        block_list_iterator_has_next(it);
        it = block_list_iterator_next(it))
      tuple_handle(tuple_build_edge(node, block_list_iterator_node(it)), 1, reg);
  }
}

void thread_run(Thread *th)
{
#ifdef STATISTICS
	if(th->id == 0)
		stats_start_exec();
#endif

  /* thread initialization */
	pthread_setspecific(thread_key, th);
	
	/* initialize nodes */
  thread_init_nodes(th);
  pthread_barrier_wait(&init_barrier);
  
  /* introduce edge tuples / neighbors */
  thread_process_edges(th);
	pthread_barrier_wait(&edge_barrier);
  
  /* introduce initial tuples */
  thread_init_tuples(th);
  pthread_barrier_wait(&tuples_barrier);
	
  /* main thread loop */
  while (true) {
#ifdef USE_WORKSTEALING
    get_work_or_idle(th);
#else
		if(th->jobs == 0) {
			pthread_mutex_lock(&th->th_mutex);
			if(th->jobs == 0) {
				thread_make_idle(th);
				while(th->jobs == 0)
					pthread_cond_wait(&th->th_cond, &th->th_mutex);
			}
			pthread_mutex_unlock(&th->th_mutex);
		}
#endif
    nodes_process(th);
  }
  
  assert(0);
}

void tuple_send(tuple_t tuple, void *rt, int delay, ref_count is_new)
{ 
	assert(TUPLE_TYPE(tuple) < NUM_TYPES);
	
	if(delay > 0) {
    fprintf(stderr, "The Multicore Virtual Machine does not support delayed tuples!\n");
    exit(EXIT_FAILURE);
	}
	
  Node *dest;
	if(rt == tuple) {
    dest = thread_node();
#ifdef STATISTICS
    stats_tuples_sent_itself(TUPLE_TYPE(tuple));
#endif
    if (dest->terminate) {
      /* going to terminate anyway */
      FREE_TUPLE(tuple);
      return;
    }
  }
	else {
#ifdef MICHAELS_COMPILER
		List *route = (List*)rt;
		dest = list_iterator_node(list_last_iterator(route));
#else
		dest = (Node*)rt;
#endif
#ifdef STATISTICS
    stats_tuples_sent(TUPLE_TYPE(tuple));
#endif
	}

  assert(dest != NULL);

#ifdef MSG_DEBUG
	Node *node = thread_node();

	if(dest == node) {
	  printf(NODE_FORMAT ": SENDING TO SELF ", node->order);
    printf("%s\n", TYPE_NAME(TUPLE_TYPE(tuple)));
		tuple_print(tuple, stdout);
		printf(" %d\n", is_new);
	} else {
		printf("node %p dest %p\n", node, dest);
	  printf(NODE_FORMAT ":" NODE_FORMAT " SENDING TO OTHER ",
			  node->order, dest->order);
		tuple_print(tuple, stdout);
		printf(" %d\n", is_new);
	}
#endif
	
  node_enqueue(dest, tuple, is_new);
}

void tuple_handle(tuple_t tuple, ref_count isNew, Register *reg)
{
	tuple_type type = TUPLE_TYPE(tuple);

	printf("New Tuple ");
	tuple_print(tuple, stdout); printf("\n");

#ifdef STATISTICS
	if(isNew > 0)
		stats_proved_tuple(type, isNew);
	else
		stats_retracted_tuple(type, -isNew);
#endif

#ifdef PAGERANK
#endif

#ifdef BELIEF_PROPAGATION
#endif

#ifdef WALKGRID
	if(TUPLE_TYPE(tuple) == type_lookup("pluggedIn") && isNew > 0) {
		printf("Sink plugged in!\n");
	}
	
	if(TUPLE_TYPE(tuple) == type_lookup("unplugged") && isNew > 0) {
    printf("Sink unplugged!\n");
	}

	if(TUPLE_TYPE(tuple) == type_lookup("load") && isNew > 0) {
		printf("New load %f!\n", MELD_FLOAT(GET_TUPLE_FIELD(tuple, 1)));
	}
	
#endif

#ifdef SHORTEST_PATH
  static tuple_type print_path_type = -1;
  
  if(print_path_type == -1)
    print_path_type = type_lookup("print_path");
  
  if(print_path_type == type) {
    printf("Got final path with distance %d\n", MELD_INT(GET_TUPLE_FIELD(tuple, 0)));
    FREE_TUPLE(tuple);
    return;
  }
#endif

#ifdef CONNECTIVITY
  if(type == type_lookup("success")) {
  }
#endif

  assert(type < NUM_TYPES);
  
  if(TYPE_IS_DELETE(type)) {
    if(isNew < 0) {
      FREE_TUPLE(tuple);
      return;
    }
    
    int delete_type = MELD_INT(GET_TUPLE_FIELD(tuple, 0));
    
    FREE_TUPLE(tuple);
    
    //printf("DELETE TYPE: %s\n", tuple_names(delete_type));
    
    if(TYPE_IS_PERSISTENT(type)) {
      fprintf(stderr, "meld: cannot delete persistent types\n");
      exit(EXIT_FAILURE);
    }
    
    tuple_queue *queue = &thread_node()->tuples[delete_type];
		tuple_entry *current = queue->head;
    tuple_entry *save;
		
		/* reset queue */
    queue_init(queue);
		
		/* delete everything */
		while (current != NULL)
		{
		  save = current->next;
		  
		  if(TYPE_IS_AGG(delete_type)) {
        tuple_queue *agg_queue = current->records.agg_queue;
        
        /* delete everything in the aggregate */
        tuple_entry *current2 = agg_queue->head;
        tuple_entry *save2;
        
        meld_free(agg_queue);
        
        while(current2 != NULL) {
          FREE_TUPLE(current2->tuple);
          save2 = current2->next;
          meld_free(current2);
          current2 = save2;
        }
        
        tuple_process(current->tuple, TYPE_START(delete_type), -1, reg);
        FREE_TUPLE(current->tuple);
			} else if (!TYPE_IS_LINEAR(delete_type)) {
			  /* non-linear deletion */
				tuple_process(current->tuple, TYPE_START(delete_type), -current->records.count, reg);
        FREE_TUPLE(current->tuple);
			} else {
				if(DELTA_WITH(type)) {
					Node* node = thread_node();

					if(node->oldTuples[type])
            FREE_TUPLE(node->oldTuples[type]);
                
          node->oldTuples[type] = current->tuple;
			  } else
					FREE_TUPLE(current->tuple);
			}
			
      meld_free(current);
      current = save;
		}
    
    return;
  } else if(TYPE_IS_SCHEDULE(type)) {
    if(isNew < 0) {
      FREE_TUPLE(tuple);
      return;
    }
    
	  tuple_type tuple_id = MELD_INT(GET_TUPLE_FIELD(tuple, 0));
    int priority = MELD_INT(GET_TUPLE_FIELD(tuple, 1));
  
    printf("SCHEDULE: id %s prio %d\n", TYPE_NAME(tuple_id), priority);
    
    FREE_TUPLE(tuple);
    return;
	}

	tuple_do_handle(type, tuple, isNew, reg);
}

static inline void
init_consts_all(void)
{
	init_consts();
	nodes_configure(NUM_TYPES);
}

void vm_init(void)
{
	srand(time(NULL));

  init_consts_all();
  init_fields();
	init_deltas();
  set_init_descriptors();
  list_init_descriptors();
	allocator_init();

#ifdef STATISTICS
	vm_add_finish_function(stats_end_exec);
	vm_add_finish_function(stats_dump);
#endif

#ifdef ALL_WORK_CHECK
  vm_add_finish_function(__finished_properly);
#endif
  
  print_program_info();
  printf("\n");
	print_program_code();
}

void vm_threads_init(void)
{
	/* init specific keys */
  pthread_key_create(&thread_key, NULL);
  
  /* init barriers */
  pthread_barrier_init(&init_barrier, NULL, NUM_THREADS);
  pthread_barrier_init(&edge_barrier, NULL, NUM_THREADS);
  pthread_barrier_init(&tuples_barrier, NULL, NUM_THREADS);

#ifdef STATISTICS
	stats_init();
#endif

	/* init thread specific allocator stuff */
	allocator_threads_init();
}

