
#ifndef NODE_H
#define NODE_H

#include <pthread.h>

#include "config.h"
#include "list.h"
#include "api.h"
#include "hash.h"

/* check for work completion */
// #define ALL_WORK_CHECK 1

/* use stratification queue */
// #define USE_STRAT_QUEUE 1

#if defined(MINIMUM_SPANNING_TREE) && !defined(USE_STRAT_QUEUE)
#define USE_STRAT_QUEUE
#endif

/* queue functions */
typedef struct _tuple_queue {
  struct _tuple_entry *head;
  struct _tuple_entry *tail;
} tuple_queue;

typedef struct _tuple_pqueue {
  struct _tuple_pentry *queue;
} tuple_pqueue;

/* tuple entry types */

typedef union {
  ref_count count;
  tuple_queue *agg_queue;
} record_type;

typedef struct _tuple_entry {
  struct _tuple_entry *next;
  record_type records;
  tuple_t tuple;
} tuple_entry;

typedef struct _tuple_pentry {
  meld_int priority;
  struct _tuple_pentry *next;
  record_type records;
  tuple_t tuple;
  void *rt;
} tuple_pentry;

/* persistent types */
typedef struct _persistent_set {
  void *array;
  int total;
  int current;
} persistent_set;

#define PERSISTENT_INITIAL 2

/* node description */
typedef enum {
	NODE_INACTIVE,
	NODE_ACTIVE,
	NODE_READY
} node_state;

typedef struct _Node Node;
struct _Node {
  struct _Node *hash_next;
  NodeID order;
  
  NodeID id;

  BlockList *neighbors;
	BlockList *first_tuples;

  struct _Thread *th; /* the node's owner */

  tuple_queue *tuples;
  persistent_set *persistent;
  
	tuple_t *oldTuples;
	
  tuple_queue queueFacts;

#ifdef USE_STRAT_QUEUE
  tuple_pqueue *newStratTuples;
#endif  

  meld_int *proved;

  pthread_mutex_t queueMutex;
  pthread_mutex_t stateMutex;
  pthread_mutex_t stratMutex;

	node_state state;
  bool terminate;

	struct _Node *prev;
	struct _Node *next;
};

Node* node_create(NodeID id);
void node_init(Node *node);
void node_add_neighbor(Node *node, Node *neighbor);
bool node_has_neighbor(Node *node, Node *neighbor);
int node_total_neighbors(Node *node);
void node_first_tuples_add(Node *node, tuple_t tuple);
void node_optimize(Node *node);
void node_clean_queues(Node *node);
void node_terminate(Node *node);

static inline void
node_first_tuples_iterate(Node *node, IterateBlockListFunction fn, void *data)
{
	block_list_iterate(node->first_tuples, fn, data);
}

static inline void
node_neighbors_iterate(Node *node, IterateBlockListFunction fn, void *data)
{
	block_list_iterate(node->neighbors, fn, data);
}

/* macro for neighbor iterators */
#define block_list_iterator_node(x) ((Node*)block_list_iterator_data(x))
/* macro for first tuples iterators */
#define block_list_iterator_tuple(x) ((tuple_t)block_list_iterator_data(x))

extern HashTable *HASH_NODES;

static inline
int nodes_total(void)
{
  return hash_table_total(HASH_NODES);
}

void nodes_init(void);
void nodes_configure(int num_types);

void nodes_add(Node *node);

static inline
Node* nodes_create(NodeID id)
{
	Node *node = node_create(id);

	nodes_add(node);

	return node;
}

static inline
Node* nodes_get(NodeID id)
{
	return hash_table_get(HASH_NODES, id);
}

static inline
Node* nodes_ensure(NodeID id)
{
  Node *node = nodes_get(id);
  
  return node ? node : nodes_create(id);
}

static inline
void nodes_iterate(HashTableIterateFunction fn, void *data)
{
	hash_table_iterate(HASH_NODES, fn, data);
}

static inline const char *get_state_name(Node *node) {
  switch(node->state) {
    case NODE_ACTIVE: return "active";
    case NODE_INACTIVE: return "inactive";
    case NODE_READY: return "ready";
  }
  
  return NULL;
}

#ifdef ALL_WORK_CHECK
void __increment_total(int total);
void __decrement_total(int total);
void __finished_properly(void);
#endif

#endif /* NODE_H */
