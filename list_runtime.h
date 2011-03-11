
#ifndef LIST_RUNTIME_H
#define LIST_RUNTIME_H

#include <stddef.h>

#include "api.h"
#include "allocator_defs.h"

typedef void* list_element;

#define LIST_NEXT(node) (*(void**)(node))
#define LIST_DATA(node) ((void*)((unsigned char*)(node) + sizeof(void*)))

typedef void (*list_print_fn)(list_element);
typedef bool (*list_equal_fn)(list_element, list_element);

typedef struct _list_descriptor
{
  size_t size_elem;
  list_print_fn print_fn;
  list_equal_fn equal_fn;
} list_descriptor;

typedef struct _List List;

struct _List {
	void *head;
	void *tail;
	unsigned short total;
  list_descriptor *descriptor;
	GC_DATA(struct _List);
};

void list_init_descriptors(void);

List *list_int_create(void);
List *list_float_create(void);
List *list_node_create(void);

void list_delete(List *list);

void list_free(List *list);

int list_equal(List *list1, List *list2);

void list_print(List *list);

void list_reverse_first(List *list);

List *list_copy(List *list);

static inline int list_total(List *list)
{
	return list->total;
}

typedef list_element list_iterator;

static inline
list_iterator list_get_iterator(List *list)
{
	return list->head;
}

static inline
list_iterator list_last_iterator(List *list)
{
	return list->tail;
}

static inline
int list_iterator_has_next(list_iterator iterator)
{
	return iterator != NULL;
}

static inline
list_iterator list_iterator_next(list_iterator iterator)
{
	return LIST_NEXT(iterator);
}

static inline
list_element list_iterator_data(list_iterator iterator)
{
	return LIST_DATA(iterator);
}

#define list_iterator_int(it) (*(meld_int *)list_iterator_data(it))
#define list_iterator_float(it) (*(meld_float *)list_iterator_data(it))
#define list_iterator_node(it) (*(Node **)list_iterator_data(it))

void list_int_push_head(List *list, meld_int data);
void list_int_push_tail(List *list, meld_int data);

void list_float_push_head(List *list, meld_float data);
void list_float_push_tail(List *list, meld_float data);

void list_node_push_head(List *list, void *data);
void list_node_push_tail(List *list, void *data);

bool list_is_float(List *list);
bool list_is_int(List *list);
bool list_is_node(List *list);

List* list_float_from_vector(meld_float *vec, int size);
List* list_int_from_vector(meld_int *vec, int size);
List* list_node_from_vector(void **vec, int size);

#endif /* LIST_RUNTIME_H */
