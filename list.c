
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "list.h"

#define NEXT_MASK 0x1
#define MAKE_NEXT_POINTER(blockListElement) \
  (block_list_element)((unsigned long int)blockListElement | NEXT_MASK)
  
#define IS_NEXT_POINTER(blockElement) \
    ((unsigned long int)blockElement & NEXT_MASK)

#define GET_NEXT_POINTER(blockElement) \
    (block_list_element*)((unsigned long int)blockElement & ~(NEXT_MASK))

BlockList *block_list_create(int size_per_element)
{
  BlockList *list = (BlockList*)meld_malloc(sizeof(BlockList));
  
  list->size_per_element = size_per_element;
  list->total = list->elems = 0;
  list->first = list->last = NULL;
  
  return list;
}

static inline block_list_element*
block_list_element_create(BlockList *list)
{
  block_list_element *vec = (block_list_element*)meld_malloc(sizeof(block_list_element) * (list->size_per_element + 1));
  
  memset(vec, 0, sizeof(block_list_element) * list->size_per_element);
  
  *(vec + list->size_per_element) = (block_list_element)NEXT_MASK;
  
  return vec;
}

void block_list_insert(BlockList *list, block_list_element elem)
{
  assert(list);
  
  if(list->first == NULL) {
    list->first = list->last = block_list_element_create(list);
    list->elems++;
  } else {
    list->last++;
    
    if(IS_NEXT_POINTER(*(list->last))) {
      block_list_element *vec = block_list_element_create(list);
    
      *(list->last) = MAKE_NEXT_POINTER(vec);
    
      list->elems++;  
      list->last = vec;
    }
  }
  
  *(list->last) = elem;
  list->total++;
}

block_list_element *block_list_get_ref(BlockList *list, int n)
{
  if (n >= list->total)
    return NULL;
  
  int elemNo = n / list->size_per_element;
  int i;
  block_list_element *vec = list->first;
  
  for(i = 0; i < elemNo; ++i)
    vec = GET_NEXT_POINTER(*(vec + list->size_per_element));
  
  return vec + (n % list->size_per_element);
}

block_list_element block_list_get(BlockList *list, int n)
{
  if(n >= list->total)
    return NULL;
    
  return *block_list_get_ref(list, n);
}

bool block_list_set(BlockList *list, int n, block_list_element elem)
{
  if (n >= list->total)
    return false;
  
  block_list_element *ref = block_list_get_ref(list, n);
  
  *ref = elem;
  
  return true;
}

int block_list_total(BlockList *list)
{
	return list->total;
}

void block_list_info(BlockList *list, FILE *fp)
{
  fprintf(fp, "BlockList %lx total elems %d total pages %d\n",
      (unsigned long int)list, list->total, list->elems);
  size_t mem = sizeof(block_list_element) * list->elems * (list->size_per_element + 1);

  fprintf(fp, "\tutilising %Zu bytes of memory\n", mem);
}

void block_list_free(BlockList *list)
{
  block_list_element *vec = list->first;

	while (vec) {
		block_list_element *next = GET_NEXT_POINTER(*(vec + list->size_per_element));

		meld_free(vec);

		vec = next;
	}

	meld_free(list);
}

void block_list_iterate(BlockList *list, IterateBlockListFunction fn,
		void *data)
{
	block_list_iterator it = block_list_get_iterator(list);

	while(block_list_iterator_has_next(it)) {

		fn(block_list_iterator_data(it), data);

		it = block_list_iterator_next(it);
	}
}

block_list_iterator
block_list_iterator_next(block_list_iterator it)
{
	it++;

	if(IS_NEXT_POINTER(*it))
		it = GET_NEXT_POINTER(*it);

	return it;
}

DoubleLinkedList *double_linked_list_create(double_linked_list_element elem,
    DoubleLinkedList *prev, DoubleLinkedList *next)
{
  DoubleLinkedList *ret = (DoubleLinkedList*)meld_malloc(sizeof(DoubleLinkedList));

  ret->elem = elem;
  ret->prev = prev;
  ret->next = next;

  if (next)
    next->prev = ret;
  if (prev)
    prev->next = ret;

  return ret;
}

void double_linked_list_remove(DoubleLinkedList *list)
{
  DoubleLinkedList *prev = list->prev;
  DoubleLinkedList *next = list->next;

  if (prev)
    prev->next = next;
  if (next)
    next->prev = prev;
}

void double_linked_list_free(DoubleLinkedList *list)
{
	while (list) {
		DoubleLinkedList *next = list->next;
		meld_free(list);
		list = next;
	}
}
