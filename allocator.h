
#ifndef ALLOCATOR_H
#define ALLOCATOR_H

/*
 * CAUTION: this is mainly an allocator + garbage collector
 * for lists & sets but may be extended in the future for other things
 */

#include <pthread.h>

#include "allocator_defs.h"
#include "list_runtime.h"
#include "set_runtime.h"
#include "api.h"
#include "thread.h"

//#define GC_BY_TIME 1

typedef struct _allocator_list_queue {
  List *head;
  List *tail;
} allocator_list_queue;

typedef struct _allocator_set_queue {
  Set *head;
  Set *tail;
} allocator_set_queue;

#ifdef GC_BY_TIME
extern bool do_gc;
#endif

typedef struct _allocator {
  allocator_list_queue *list;
  pthread_mutex_t *list_mutex;
  
  allocator_set_queue *set;
  pthread_mutex_t *set_mutex;
  
#ifdef GC_BY_TIME
	big_counter cycles;
#else
	big_counter allocated_lists;
	big_counter next_check_lists;
	
  big_counter allocated_sets;
  big_counter next_check_sets;
#endif
} Allocator;

List* allocator_allocate_list(void);

Set* allocator_allocate_set(void);

void allocator_init(void);
void allocator_threads_init(void);

#ifndef GC_BY_TIME
static inline bool
allocator_will_collect_lists(void)
{
  extern Allocator alloc;
  return alloc.allocated_lists > alloc.next_check_lists;
}

static inline bool
allocator_will_collect_sets(void)
{
  extern Allocator alloc;
  return alloc.allocated_sets > alloc.next_check_sets;
}
#endif

static inline bool
allocator_will_collect(Thread *th, const bool do_yes)
{
#ifdef GC_BY_TIME
	extern Allocator alloc;

	if(th->id == 0 && do_yes) {
		alloc.cycles++;

		if(alloc.cycles > 100000) {
			do_gc = true;
			return true;
		}
	}

	return do_gc;

#else
	(void)th;
	(void)do_yes;
  return allocator_will_collect_sets() || allocator_will_collect_lists();
#endif
}

void allocator_collect(Thread *th);

#endif /* ALLOCATOR_H */

