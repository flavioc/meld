
#ifndef LIST_H
#define LIST_H

#include <stdio.h>

#include "defs.h"

/*************************
 **     Block List      **
 *************************/

typedef void* block_list_element;

typedef struct _BlockList {
  block_list_element *first;
  block_list_element *last;
  int size_per_element;
  int elems;
  int total;
} BlockList;

BlockList *block_list_create(int size_per_element);

void block_list_insert(BlockList *list, block_list_element elem);

block_list_element *block_list_get_ref(BlockList *list, int n);

block_list_element block_list_get(BlockList *list, int n);

bool block_list_set(BlockList *list, int n, block_list_element elem);

int block_list_total(BlockList *list);

void block_list_info(BlockList *list, FILE *fp);

void block_list_free(BlockList *list);

typedef void (*IterateBlockListFunction)(block_list_element, void*);

void block_list_iterate(BlockList *list, IterateBlockListFunction fn,
		void *data);

typedef block_list_element* block_list_iterator;

static inline block_list_iterator
block_list_get_iterator(BlockList *list)
{
	return list->first;
}

static inline bool
block_list_iterator_has_next(block_list_iterator it)
{
	return it != NULL && *it != NULL;
}

block_list_iterator block_list_iterator_next(block_list_iterator it);

static inline block_list_element
block_list_iterator_data(block_list_iterator it)
{
	return *it;
}

/*******************************
 **    Double Linked List     **
 *******************************/

typedef void* double_linked_list_element;

typedef struct _DoubleLinkedList {
  double_linked_list_element elem;
  
  struct _DoubleLinkedList *prev;
  struct _DoubleLinkedList *next;
} DoubleLinkedList;

DoubleLinkedList *double_linked_list_create(double_linked_list_element elem,
    DoubleLinkedList *prev, DoubleLinkedList *next);

void double_linked_list_remove(DoubleLinkedList *list);

void double_linked_list_free(DoubleLinkedList *list);

#endif /* LIST_H */
