
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "list_runtime.h"
#include "api.h"
#include "model.h"
#include "config.h"
#include "allocator.h"

static list_descriptor *int_descriptor = NULL;
static list_descriptor *float_descriptor = NULL;
static list_descriptor *node_descriptor = NULL;

unsigned long int saved_ctn = 0;

static inline List*
list_create(list_descriptor *descriptor)
{
	List *ret = allocator_allocate_list();

	ret->total = 0;
  ret->descriptor = descriptor;
	ret->head = ret->tail = NULL;
	
#ifdef STATISTICS
  if(descriptor == int_descriptor)
    stats_lists_allocated(LIST_INT);
  else if(descriptor == float_descriptor)
    stats_lists_allocated(LIST_FLOAT);
  else if(descriptor == node_descriptor)
    stats_lists_allocated(LIST_NODE);
#endif /* STATISTICS */

	return ret;
}

static inline void*
list_create_node(List *list, list_element data, void *next)
{
	void *node = (void*)meld_malloc(sizeof(void*) + list->descriptor->size_elem);

	LIST_NEXT(node) = next;

	memcpy(LIST_DATA(node), data, list->descriptor->size_elem);

	return node;
}

static inline void
list_push_head(List *list, list_element data)
{
	void *node = list_create_node(list, data, list->head);

	list->head = node;

	if(list->tail == NULL)
		list->tail = node;

	list->total++;
}

static inline void
list_push_tail_node(List *list, void *node)
{
	if(list->tail != NULL)
		LIST_NEXT(list->tail) = node;
	list->tail = node;
	if(list->head == NULL)
		list->head = node;
	LIST_NEXT(node) = NULL;
	list->total++;
}

static inline void
list_push_tail(List *list, list_element data)
{
	list_push_tail_node(list, list_create_node(list, data, NULL));
}

static inline void*
list_pop_head(List *list)
{
	if(list->head != NULL) {
		void *ptr = list->head;

		list->head = LIST_NEXT(ptr);

		if(list->head == NULL)
			list->tail = NULL;

		--list->total;

		return ptr;
	} else
		return NULL;
}

List *list_int_create(void)
{
  assert(int_descriptor != NULL);

	return list_create(int_descriptor);
}

List *list_float_create(void)
{
  assert(float_descriptor != NULL);
  
	return list_create(float_descriptor);
}

List *list_node_create(void)
{
	assert(node_descriptor != NULL);
	
	return list_create(node_descriptor);
}

void list_int_push_head(List *list, meld_int data)
{
  list_push_head(list, (list_element)&data);
}

void list_int_push_tail(List *list, meld_int data)
{
	list_push_tail(list, (list_element)&data);
}

void list_int_pop_head(List *list)
{
	list_pop_head(list);
}

void list_node_push_head(List *list, void *data)
{
	list_push_head(list, (list_element)&data);
}

void list_node_push_tail(List *list, void *data)
{
	list_push_tail(list, (list_element)&data);
}

void list_float_push_head(List *list, meld_float data)
{
  list_push_head(list, (list_element)&data);
}

void list_float_push_tail(List *list, meld_float data)
{
	list_push_tail(list, (list_element)&data);
}

bool list_is_float(List *list)
{
	return list->descriptor == float_descriptor;
}

bool list_is_int(List *list)
{
	return list->descriptor == int_descriptor;
}

bool list_is_node(List *list)
{
	return list->descriptor == node_descriptor;
}

List* list_int_from_vector(meld_int *vec, int size)
{
	List *list = list_int_create();
	int i;

	for(i = 0; i < size; ++i)
		list_int_push_tail(list, vec[i]);

	return list;
}

List* list_float_from_vector(meld_float *vec, int size)
{
	List *list = list_float_create();
	int i;

	for(i = 0; i < size; ++i)
		list_float_push_tail(list, vec[i]);

	return list;
}

List* list_node_from_vector(void **vec, int size)
{
	List *list = list_node_create();
	int i;

	for(i = 0; i < size; ++i) {
		list_node_push_tail(list, vec[i]);
	}

	return list;
}

void list_free(List *list)
{
	void *chain = list->head;
	void *next;
	
	while(chain) {
		next = LIST_NEXT(chain);
		meld_free(chain);
		chain = next;
	}
	meld_free(list);
}

void list_delete(List *list)
{
	(void)list;
	/* this doesn't do anything really */
}

int list_equal(List *list1, List *list2)
{
  if(list1->descriptor != list2->descriptor)
    return 0;
    
	if(list_total(list1) != list_total(list2))
		return 0;
	
	list_iterator it1 = list_get_iterator(list1);
	list_iterator it2 = list_get_iterator(list2);

	while(list_iterator_has_next(it1)) {
	  if(!list1->descriptor->equal_fn(list_iterator_data(it1), list_iterator_data(it2)))
			return 0;

		it1 = list_iterator_next(list1);
		it2 = list_iterator_next(list2);
	}

	return 1; /* they are equal! */
}

void list_print(List *list, FILE *out)
{
	if(list == NULL) {
		fprintf(out, "LIST(NULL:0):[]");
	} else {
		list_iterator it = list_get_iterator(list);

		fprintf(out, "LIST(%p:%d):[", list, list->total);

		while (list_iterator_has_next(it)) {
			list->descriptor->print_fn(list_iterator_data(it), out);
			it = list_iterator_next(it);
			if (list_iterator_has_next(it))
				fprintf(out, ", ");
		}
		fprintf(out, "]");
	}
}

void list_reverse_first(List *list)
{
	void *node = list_pop_head(list);

	if (node) {
		list_push_tail_node(list, node);
	}
}

List* list_copy(List *list)
{
	List *clone = list_create(list->descriptor);
	list_iterator it = list_get_iterator(list);

	while(list_iterator_has_next(it)) {
		list_push_tail(clone, list_iterator_data(it));

		it = list_iterator_next(it);
	}

	return clone;
}

static void
print_int_list_elem(list_element data, FILE *out)
{
  fprintf(out, "%d", MELD_INT(data));
}

static bool
equal_int_list_elem(list_element el1, list_element el2)
{
  return MELD_INT(el1) == MELD_INT(el2);
}

static void
print_float_list_elem(list_element data, FILE *out)
{
  fprintf(out, "%f", MELD_FLOAT(data));
}

static bool
equal_float_list_elem(list_element el1, list_element el2)
{
  return MELD_FLOAT(el1) == MELD_FLOAT(el2);
}

static void
print_node_list_elem(list_element data, FILE *out)
{
#ifdef PARALLEL_MACHINE
	fprintf(out, NODE_FORMAT, MELD_NODE(data)->order);
  fprintf(out, " %p", MELD_NODE(data));
#else
	fprintf(out, NODE_FORMAT, MELD_NODE(data)->id);
#endif
}

static bool
equal_node_list_elem(list_element el1, list_element el2)
{
	return MELD_PTR(el1) == MELD_PTR(el2);
}

void
list_init_descriptors(void)
{
  int_descriptor = (list_descriptor*)meld_malloc(sizeof(list_descriptor));
  int_descriptor->size_elem = sizeof(meld_int);
  int_descriptor->print_fn = print_int_list_elem;
  int_descriptor->equal_fn = equal_int_list_elem;
  
  float_descriptor = (list_descriptor*)meld_malloc(sizeof(list_descriptor));
  float_descriptor->size_elem = sizeof(meld_float);
  float_descriptor->print_fn = print_float_list_elem;
  float_descriptor->equal_fn = equal_float_list_elem;

	node_descriptor = (list_descriptor*)meld_malloc(sizeof(list_descriptor));
	node_descriptor->size_elem = sizeof(void*);
	node_descriptor->print_fn = print_node_list_elem;
	node_descriptor->equal_fn = equal_node_list_elem;
}
