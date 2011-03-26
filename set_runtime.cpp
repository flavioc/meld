
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "set_runtime.h"
#include "model.h"
#include "allocator.h"

static set_descriptor *int_descriptor = NULL;
static set_descriptor *float_descriptor = NULL;

static inline Set*
set_create(set_descriptor *descriptor)
{
	Set* set = allocator_allocate_set();

	set->start = NULL;
	set->nelems = 0;
  set->descriptor = descriptor;
  
#ifdef STATISTICS
  if(descriptor == int_descriptor)
    stats_sets_allocated(SET_INT);
  else if(descriptor == float_descriptor)
    stats_sets_allocated(SET_FLOAT);
#endif

	return set;
}

#define ALLOC_AND_SET(set, data, next, dest)                                        \
	dest = (unsigned char*)meld_malloc(set->descriptor->size_elem + sizeof(void*));                   \
	memcpy(dest, data, set->descriptor->size_elem);                                   \
  *((void**)((unsigned char *)(dest) + set->descriptor->size_elem)) = next;         \
  set->nelems++

#define ADVANCE_NEXT(set, ptr) *(unsigned char**)(ptr + set->descriptor->size_elem)

void
set_insert(Set *set, set_data data)
{
	unsigned char *before = NULL;
	unsigned char *current = set->start;

	while (current) {
		set_data elem = current;

		if(set->descriptor->cmp_fn(data, elem)) {
			/* insert it here */

			/* is it repeated? */
			if(before && set->descriptor->equal_fn(data, before))
				return;

			if (before) {
				void **beforenext = (void**)(before + set->descriptor->size_elem);
				ALLOC_AND_SET(set, data, current, *beforenext);
			} else {
				ALLOC_AND_SET(set, data, current, set->start);
			}

			return;
		}

		/* go to next */
		before = current;
		current = ADVANCE_NEXT(set, current);
	}

	if (before) {
		/* repeated? */
		if (set->descriptor->equal_fn(data, before))
			return;

		void **beforenext = (void**)(before + set->descriptor->size_elem);
		ALLOC_AND_SET(set, data, NULL, *beforenext);
	} else {
		ALLOC_AND_SET(set, data, NULL, set->start);
	}
}

static bool
compare_int_values(set_data a1, set_data a2)
{
	meld_int i1 = MELD_INT(a1);
	meld_int i2 = MELD_INT(a2);

	return i1 < i2;
}

static bool
equal_int_values(set_data a1, set_data a2)
{
	meld_int i1 = MELD_INT(a1);
	meld_int i2 = MELD_INT(a2);

	return i1 == i2;
}

static void
print_int_value(set_data a)
{
	printf("%d", MELD_INT(a));
}

Set*
set_int_create(void)
{
  assert(int_descriptor != NULL);
	return set_create(int_descriptor);
}

static bool
compare_float_values(set_data a1, set_data a2)
{
	meld_float f1 = MELD_FLOAT(a1);
	meld_float f2 = MELD_FLOAT(a2);

	return f1 < f2;
}

static bool
equal_float_values(set_data a1, set_data a2)
{
	meld_float f1 = MELD_FLOAT(a1);
	meld_float f2 = MELD_FLOAT(a2);

	return f1 == f2;
}

static void
print_float_value(set_data a)
{
	printf("%f", MELD_FLOAT(a));
}

Set*
set_float_create(void)
{
  assert(float_descriptor != NULL);
	return set_create(float_descriptor);
}

void
set_int_insert(Set* set, int data)
{
	set_insert(set, (set_data)&data);
}

void
set_float_insert(Set* set, float data)
{
	set_insert(set, (set_data)&data);
}

void
set_print(Set *set)
{
	printf("(Set-Union with %d elems, %Zu bytes each [", set->nelems, set->descriptor->size_elem);

	unsigned char* current = set->start;
	int isFirst = 1;

	while(current) {

		if(isFirst)
			isFirst = 0;
		else
			printf(", ");

		set->descriptor->print_fn((set_data)current);

		current = ADVANCE_NEXT(set, current);
	}

	printf("])\n");
}

bool set_equal(Set *set1, Set *set2)
{
	if(set1->nelems != set2->nelems)
		return false;

	if(set1->descriptor != set2->descriptor)
		return false;

	unsigned char* current1 = set1->start;
	unsigned char* current2 = set2->start;

	while (current1) {
		set_data elem1 = (set_data)current1;
		set_data elem2 = (set_data)current2;

		if(!set1->descriptor->equal_fn(elem1, elem2))
			return false;

		current1 = ADVANCE_NEXT(set1, current1);
		current2 = ADVANCE_NEXT(set1, current2);
	}

	return true;
}

void set_delete(Set *set)
{
  (void)set;
}

void set_free(Set *set)
{
  unsigned char* current = set->start;
	unsigned char* next;

	while (current) {
		
		next = ADVANCE_NEXT(set, current);
		meld_free(current);
		current = next;
	}

	meld_free(set);
}

Set* set_copy(Set *set)
{
  Set *copy = set_create(set->descriptor);
  
  unsigned char* current = set->start;
  
  while (current) {
    set_data data = (set_data)current;
    
    set_insert(copy, data);
    
    current = ADVANCE_NEXT(set, current);
  }
  
  return copy;
}

bool set_is_float(Set *set)
{
	return set->descriptor == float_descriptor;
}

bool set_is_int(Set *set)
{
	return set->descriptor == int_descriptor;
}

void
set_init_descriptors(void)
{
  int_descriptor = (set_descriptor*)meld_malloc(sizeof(set_descriptor));
  int_descriptor->size_elem = sizeof(int);
  int_descriptor->print_fn = print_int_value;
  int_descriptor->cmp_fn = compare_int_values;
  int_descriptor->equal_fn = equal_int_values;
  
  float_descriptor = (set_descriptor*)meld_malloc(sizeof(set_descriptor));
  float_descriptor->size_elem = sizeof(float);
  float_descriptor->print_fn = print_float_value;
  float_descriptor->cmp_fn = compare_float_values;
  float_descriptor->equal_fn = equal_float_values;
}
