
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "utils.h"
#include "allocator.h"
#include "barrier.h"
#include "thread.h"
#include "node.h"
#include "core.h"
#include "hash.h"
#include "stats.h"

#define GC_DEBUG 1
#define ALLOCATOR_FREQUENCY 250000

#ifdef GC_BY_TIME
bool do_gc = false;
#endif

Allocator alloc;
static pthread_barrier_t gc_initial_barrier;
static pthread_barrier_t gc_mark_barrier;
static pthread_barrier_t gc_sweep_barrier;
static pthread_barrier_t gc_final_barrier;

void
allocator_init(void)
{
#ifdef GC_BY_TIME
	alloc.cycles = 0;
#else
	alloc.allocated_lists = 0;
	alloc.next_check_lists = ALLOCATOR_FREQUENCY;
	
  alloc.allocated_sets = 0;
  alloc.next_check_sets = ALLOCATOR_FREQUENCY;
#endif
}

void
allocator_threads_init(void)
{
	pthread_barrier_init(&gc_initial_barrier, NULL, NUM_THREADS);
	pthread_barrier_init(&gc_mark_barrier, NULL, NUM_THREADS);
	pthread_barrier_init(&gc_sweep_barrier, NULL, NUM_THREADS);
	pthread_barrier_init(&gc_final_barrier, NULL, NUM_THREADS);
	
  alloc.list = meld_malloc(sizeof(allocator_list_queue)*NUM_THREADS);
  memset(alloc.list, 0, sizeof(allocator_list_queue)*NUM_THREADS);
  alloc.set = meld_malloc(sizeof(allocator_set_queue)*NUM_THREADS);
  memset(alloc.set, 0, sizeof(allocator_set_queue)*NUM_THREADS);
  
  alloc.list_mutex = meld_malloc(sizeof(pthread_mutex_t)*NUM_THREADS);
  alloc.set_mutex = meld_malloc(sizeof(pthread_mutex_t)*NUM_THREADS);
  
  int i;
  for(i = 0; i < NUM_THREADS; ++i) {
    pthread_mutex_init(&alloc.list_mutex[i], NULL);
    pthread_mutex_init(&alloc.set_mutex[i], NULL);
  }
}

#ifndef GC_BY_TIME
static inline void
increment_list_count(const int total)
{
	__sync_add_and_fetch(&alloc.allocated_lists, total);
}

static inline void
decrement_list_count(const int total)
{
	__sync_sub_and_fetch(&alloc.allocated_lists, total);
}

static inline void
increment_set_count(const int total)
{
	__sync_add_and_fetch(&alloc.allocated_sets, total);
}

static inline void
decrement_set_count(const int total)
{
	__sync_sub_and_fetch(&alloc.allocated_sets, total);
}
#endif

List*
allocator_allocate_list(void)
{
	List *ret = meld_malloc(sizeof(List));
	
  const int pos = random_int(NUM_THREADS);
  allocator_list_queue *queue = &alloc.list[pos];

	pthread_mutex_lock(&alloc.list_mutex[pos]);

  GC_SET_NEXT(ret, queue->head);
  
	if(queue->head == NULL)
		queue->tail = ret;
  queue->head = ret;

	pthread_mutex_unlock(&alloc.list_mutex[pos]);

#ifndef GC_BY_TIME
	increment_list_count(1);
#endif

	return ret;
}

Set*
allocator_allocate_set(void)
{
  Set *ret = meld_malloc(sizeof(Set));
  
  const int pos = random_int(NUM_THREADS);
  
  allocator_set_queue *queue = &alloc.set[pos];
  
  pthread_mutex_lock(&alloc.set_mutex[pos]);
  
  GC_SET_NEXT(ret, queue->head);
  
  if(queue->head == NULL)
    queue->tail = ret;
  queue->head = ret;
  
  pthread_mutex_unlock(&alloc.set_mutex[pos]);
  
#ifndef GC_BY_TIME
  increment_set_count(1);
#endif
  
  return ret;
}

static inline void
tuple_mark(tuple_type type, tuple_t tuple, bool do_lists, bool do_sets)
{
	int i;

	for(i = 0; i < TYPE_NOARGS(type); ++i) {
		switch(TYPE_ARG_TYPE(type, i)) {
			case FIELD_LIST_INT:
			case FIELD_LIST_FLOAT:
			case FIELD_LIST_ADDR: {
			  if(!do_lists)
          break;
				List *list = MELD_LIST(GET_TUPLE_FIELD(tuple, i));
				GC_MARK_OBJECT(List, list);
				break;
			}
			case FIELD_SET_INT:
			case FIELD_SET_FLOAT: {
			  if(!do_sets)
          break;
        Set *set = MELD_SET(GET_TUPLE_FIELD(tuple, i));
        GC_MARK_OBJECT(Set, set);
        break;
			}
			default:
				break;
		}
	}
}

static void
node_mark(Node *node, bool do_lists, bool do_sets)
{
	/* mark database of facts */
	tuple_type i;
	for(i = 0; i < NUM_TYPES; ++i) {
		if(TYPE_IS_PERSISTENT(i)) {
			persistent_set *persistent = &node->persistent[i];
			int size = TYPE_SIZE(i);
			int j;

			for(j = 0; j < persistent->current; ++j) {
				tuple_t tuple = persistent->array + j * size;

				tuple_mark(i, tuple, do_lists, do_sets);
			}
		} else {
			tuple_entry *entry = node->tuples[i].head;

			while (entry) {
				tuple_t tuple = entry->tuple;

				tuple_mark(i, tuple, do_lists, do_sets);

				if(TYPE_IS_AGG(i)) {
					tuple_queue *agg_queue = entry->records.agg_queue;
					tuple_entry *agg = agg_queue->head;

					while(agg) {
						tuple_t agg_tuple = agg->tuple;

						tuple_mark(i, agg_tuple, do_lists, do_sets);

						agg = agg->next;
					}
				}

				entry = entry->next;
			}
		}
	}

	/* mark queues */
	tuple_entry *new_facts = node->queueFacts.head;
	while(new_facts) {
		tuple_t tuple = new_facts->tuple;
		tuple_mark(TUPLE_TYPE(tuple), tuple, do_lists, do_sets);
		new_facts = new_facts->next;
	}

#ifdef USE_STRAT_QUEUE
	tuple_pentry *pentry = node->newStratTuples->queue;
	while(pentry) {
		tuple_t tuple = pentry->tuple;

		tuple_mark(TUPLE_TYPE(tuple), tuple, do_lists, do_sets);

		pentry = pentry->next;
	}
#endif

	/* mark old tuples for delta */
	for(i = 0; i < NUM_TYPES; ++i) {
		tuple_t tuple = node->oldTuples[i];
		if(tuple != NULL)
			tuple_mark(i, tuple, do_lists, do_sets);
	}
}

static inline void
list_collect(List *list)
{
#ifdef STATISTICS
	if(list_is_int(list))
		stats_lists_collected(LIST_INT);
	else if(list_is_float(list))
		stats_lists_collected(LIST_FLOAT);
	else if(list_is_node(list))
		stats_lists_collected(LIST_NODE);
#endif
	list_free(list);
}

static big_counter
sweeper_list_sweep(allocator_list_queue *queue)
{
  big_counter removed = 0;
	List *list = queue->head;
	
  queue->head = NULL;
  queue->tail = NULL;
	
	while(list) {
		List *next = GC_GET_NEXT(List, list);
		
		if(!GC_OBJECT_MARKED(list)) {	
      removed++;
			list_collect(list);
		} else {
		  /* object is marked by GC_SET_NEXT ... */
			GC_SET_NEXT(list, queue->head);

    	if(queue->head == NULL)
    		queue->tail = list;
      queue->head = list;
		}
		
    list = next;
	}
	
  return removed;
}

static inline void
set_collect(Set *set)
{
#ifdef STATISTICS
	if(set_is_int(set))
		stats_sets_collected(SET_INT);
	else if(set_is_float(set))
		stats_sets_collected(SET_FLOAT);
#endif
	set_free(set);
}

static big_counter
sweeper_set_sweep(allocator_set_queue *queue)
{
  big_counter removed = 0;
	Set *set = queue->head;
	
  queue->head = NULL;
  queue->tail = NULL;
	
	while(set) {
		Set *next = GC_GET_NEXT(Set, set);
		
		if(!GC_OBJECT_MARKED(set)) {	
      removed++;
			set_collect(set);
		} else {
		  /* object is marked by GC_SET_NEXT ... */
			GC_SET_NEXT(set, queue->head);

    	if(queue->head == NULL)
    		queue->tail = set;
      queue->head = set;
		}
		
    set = next;
	}
	
  return removed;
}

void
allocator_collect(Thread *th)
{ 
#if defined(STATISTICS) || defined(GC_DEBUG)
	clock_t start = 0, end;

	if(th->id == 0)
		start = clock();
#endif

	pthread_barrier_wait(&gc_initial_barrier);
	
#ifdef GC_DEBUG
  printf("%d: GARBAGE COLLECTING STARTED\n", th->id);
#endif
	
#ifdef GC_BY_TIME
	const bool do_lists = true;
	const bool do_sets = true;
#else
  const bool do_lists = allocator_will_collect_lists();
  const bool do_sets = allocator_will_collect_sets();
#endif

	block_list_iterator it = block_list_get_iterator(th->gc_nodes);

	while (block_list_iterator_has_next(it)) {
		Node *node = block_list_iterator_node(it);

		node_mark(node, do_lists, do_sets);

		it = block_list_iterator_next(it);
	}

	pthread_barrier_wait(&gc_mark_barrier);

  if(do_lists) {
    allocator_list_queue *queue = &alloc.list[th->id];
    big_counter list_count = sweeper_list_sweep(queue);
#ifdef GC_DEBUG
    printf("%d: COLLECTED %ld lists\n", th->id, list_count);
#endif
#ifndef GC_BY_TIME
    decrement_list_count(list_count);
#endif
  }
  if(do_sets) {
    allocator_set_queue *queue = &alloc.set[th->id];
    big_counter set_count = sweeper_set_sweep(queue);
#ifdef GC_DEBUG
    printf("%d: COLLECTED %ld sets\n", th->id, set_count);
#endif
#ifndef GC_BY_TIME
    decrement_set_count(set_count);
#endif
  }

	pthread_barrier_wait(&gc_sweep_barrier);
	
	if(th->id == 0) {
#ifdef GC_BY_TIME
		alloc.cycles = 0;
		do_gc = false;
#else
	  if(do_lists)
		  alloc.next_check_lists = alloc.allocated_lists + ALLOCATOR_FREQUENCY;
	  if(do_sets)
      alloc.next_check_sets = alloc.allocated_sets + ALLOCATOR_FREQUENCY;
#endif
	}
  
#ifdef GC_DEBUG
	printf("%d: GARBAGE_COLLECTION DONE\n", th->id);
#endif
	
  pthread_barrier_wait(&gc_final_barrier);

#if defined(STATISTICS) || defined(GC_DEBUG)
	if(th->id == 0) {
#ifdef STATISTICS
		stats_run_gc();
#endif

		end = clock();

		const double elapsed = ((double) (end - start)) / (CLOCKS_PER_SEC / 1000);

#ifdef GC_DEBUG
    printf("GARBAGE COLLECTING TOOK %.3lf ms\n", elapsed);
#endif

#ifdef STATISTICS
		stats_time_gc(elapsed);
#endif
	}
#endif /* STATISTICS || GC_DEBUG */
}
