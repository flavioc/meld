
#include <stdlib.h>

#include "utils.h"
#include "config.h"
#include "barrier.h"
#include "partition.h"

#ifdef COMPLEX_PARTITION

static pthread_barrier_t barrier;

typedef struct _Queue {
	DoubleLinkedList *head;
	DoubleLinkedList *tail;
} Queue;

static void
neighborhood_iterate(block_list_element elem, void *data)
{
	Queue *queue = (Queue*)data;
	Node *node = (Node*)elem;

	DoubleLinkedList *new_node = double_linked_list_create(
			(double_linked_list_element)node,
			queue->tail,
			NULL);

	queue->tail = new_node;
	if(!queue->head)
		queue->head = queue->tail;
}

static inline void
neighbors_add(Node *node, Queue *queue)
{
	node_neighbors_iterate(node, neighborhood_iterate, queue);
}

static void
queue_free(Queue *queue)
{
	if(queue->head)
		double_linked_list_free(queue->head);
	meld_free(queue);
}

typedef struct {
  Thread *th;
  int total;
} IterateData;

static bool
iterate_nodes(hash_table_element elem, void *_data)
{
  IterateData *data = (IterateData*)_data;
	Thread *th = data->th;
	Node *node = (Node*)elem;
	Queue *queue = NULL;

	if(pthread_mutex_trylock(&node->queueMutex) == 0) {
		/* node is available */

    thread_add_initial_node(th, node);
		if(--data->total == 0)
			return false;

		queue = (Queue*)meld_malloc(sizeof(Queue));
		queue->tail = queue->head = NULL;

		neighbors_add(node, queue);

		while (queue->head) {
			node = (Node*)queue->head->elem;

			if (pthread_mutex_trylock(&node->queueMutex) == 0) {
        thread_add_initial_node(th, node);
				if(--data->total == 0) {
					queue_free(queue);
					return false;
				}

				neighbors_add(node, queue);
			}
			DoubleLinkedList *next = queue->head->next;
			meld_free(queue->head);
			queue->head = next;
		}

		queue_free(queue);
	}

	return true;
}

void
partition_init(int nThreads)
{
  pthread_barrier_init(&barrier, NULL, nThreads);
}

void
partition_do(HashTable *nodes, Thread *th, int start, int total)
{
  (void)start;
  
  IterateData *data = (IterateData*)meld_malloc(sizeof(IterateData));
  data->th = th;
  data->total = total;
  
	hash_table_iterate(nodes, iterate_nodes, (void *)data);

  meld_free(data);
}

void
partition_finish(Thread *th)
{
  pthread_barrier_wait(&barrier);
  
	ITERATE_INITIAL_QUEUE(th, node) {
		pthread_mutex_unlock(&node->queueMutex);
	}
}

#else

void
partition_init(int nThreads)
{
	(void)nThreads;
  /* nothing */
}

void
partition_do(HashTable *nodes, Thread *th, int start, int total)
{
  int i;
  Node *node;
  
  for(i = 0; i < total; ++i) {
    node = hash_table_get(nodes, i + start);
    thread_add_initial_node(th, node);
  }
}

void
partition_finish(Thread *th)
{
	(void)th;
  /* nothing */
}

#endif /* COMPLEX_PARTITION */
