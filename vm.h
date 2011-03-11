
#ifndef MELDVM_H
#define MELDVM_H

#include "thread.h"
#include "barrier.h"
#include "core.h"
#include "list_runtime.h"

void vm_init(void);
void vm_threads_init(void);
void thread_run(Thread *th);
void node_assert(Node *node, tuple_t tuple);
tuple_type type_lookup(const char *name);
List* route_build_direct(Node *p1, Node *p2);
tuple_t tuple_build_edge(Node *a, Node *b);

#endif /* MELDVM_H */
