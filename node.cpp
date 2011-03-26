
#include <stdlib.h>
#include <assert.h>

#include "config.h"
#include "node.h"
#include "hash.h"
#include "core.h"
#include "thread.h"

HashTable *HASH_NODES;
static int num_types = 0;

Node*
node_create(NodeID id)
{
	Node *node = (Node *)meld_malloc(sizeof(Node));

	node->id = (NodeID)node;
	node->order = id;
	node->first_tuples = block_list_create(8);
  node->neighbors = block_list_create(8);
  node->state = NODE_INACTIVE;
  node->terminate = false;
  
	pthread_mutex_init(&(node->queueMutex), NULL);
  pthread_mutex_init(&(node->stateMutex), NULL);
  pthread_mutex_init(&(node->stratMutex), NULL);

	return node;
}

void
node_add_neighbor(Node *node, Node *neighbor)
{
	block_list_insert(node->neighbors, neighbor);
}
 
bool
node_has_neighbor(Node *src, Node *nei)
{
	block_list_iterator it = block_list_get_iterator(src->neighbors);

	for( ; block_list_iterator_has_next(it) ;
			it = block_list_iterator_next(it))
	{
		Node *n = (Node*)block_list_iterator_data(it);

		if(n == nei)
			return true;
	}
	
	return false;
}

int
node_total_neighbors(Node *node)
{
	return block_list_total(node->neighbors);
}

void
node_first_tuples_add(Node *node, tuple_t tuple)
{
	block_list_insert(node->first_tuples, tuple);
}

void
node_init(Node *node)
{
	node->tuples = (tuple_queue*)meld_calloc(num_types, sizeof(tuple_queue));
	node->oldTuples = (void**)meld_calloc(num_types, sizeof(tuple_t));
  node->persistent = (persistent_set*)meld_calloc(num_types, sizeof(persistent_set));
#ifdef USE_STRAT_QUEUE
  node->newStratTuples = meld_calloc(1, sizeof(tuple_pqueue));
#endif
  node->proved = (meld_int*)meld_calloc(num_types, sizeof(meld_int));
	node->state = NODE_READY;
  queue_init(&node->queueFacts);
}

void
node_optimize(Node *node)
{
  block_list_free(node->neighbors);
  block_list_free(node->first_tuples);
}

static inline void
node_clean_tuple_queue(Node *node)
{
  pthread_mutex_lock(&node->queueMutex);
  
  tuple_entry *entry = node->tuples->head;
  
  while(entry) {
    tuple_entry *next = entry->next;
    
    FREE_TUPLE(entry->tuple);
    meld_free(entry);
    
    entry = next;
  }
  
  queue_init(node->tuples);
  
  pthread_mutex_unlock(&node->queueMutex);
}

#ifdef USE_STRAT_QUEUE
static inline void
node_clean_strat_queue(Node *node)
{
  pthread_mutex_lock(&node->stratMutex);
  
  tuple_pentry *pentry = node->newStratTuples->queue;
  
  while (pentry) {
    tuple_pentry *next = pentry->next;
    
    FREE_TUPLE(pentry->tuple);
    
    meld_free(pentry);
    
    pentry = next;
  }
  
  node->newStratTuples->queue = NULL;

  pthread_mutex_unlock(&node->stratMutex);
}
#endif

void
node_clean_queues(Node *node)
{
  node_clean_tuple_queue(node);
#ifdef USE_STRAT_QUEUE
  node_clean_strat_queue(node);
#endif
}

void
node_terminate(Node *node)
{
  node->terminate = true;
}

void
nodes_init(void)
{
	HASH_NODES = hash_table_create(8);
}

void
nodes_configure(int _num_types)
{
	num_types = _num_types;
}

void
nodes_add(Node *node)
{
	hash_table_insert(HASH_NODES, node);
}

#ifdef ALL_WORK_CHECK
static volatile int totalNew = 0;

void __increment_total(int total)
{
	__sync_add_and_fetch(&totalNew, total);
}

void __decrement_total(int total)
{
	__sync_sub_and_fetch(&totalNew, total);
}

void
__finished_properly(void)
{
	if(totalNew != 0)
		printf("Total new %d\n", totalNew);
  assert(totalNew == 0);
}
#endif
