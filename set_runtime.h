
#ifndef SET_RUNTIME_H
#define SET_RUNTIME_H

#include <stddef.h>

#include "defs.h"
#include "api.h"
#include "allocator_defs.h"

typedef void* set_data;

typedef bool (*compare_set_fn)(set_data, set_data);
typedef void (*print_set_fn)(set_data);

typedef struct _set_descriptor {
  compare_set_fn cmp_fn;
  compare_set_fn equal_fn;
  print_set_fn print_fn;
  size_t size_elem;
} set_descriptor;

typedef struct _Set {
	unsigned char* start;
	int nelems;
  set_descriptor *descriptor;
  GC_DATA(struct _Set);
} Set;

void set_init_descriptors(void);

Set *set_int_create(void);
Set *set_float_create(void);

void set_insert(Set *set, set_data data);
void set_int_insert(Set *set, meld_int data);
void set_float_insert(Set *set, meld_float data);

bool set_equal(Set *set1, Set *set2);

void set_delete(Set *set);

Set* set_copy(Set *set);

void set_free(Set *set);

void set_print(Set *set);

static inline
int set_total(Set *set)
{
	return set->nelems;
}

bool set_is_int(Set *set);
bool set_is_float(Set *set);

#endif /* SET_RUNTIME_H */
