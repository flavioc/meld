
#ifndef PARTITION_H
#define PARTITION_H

#include "hash.h"
#include "node.h"
#include "list.h"
#include "thread.h"

void partition_init(int nThreads);

void partition_do(HashTable *nodes, Thread *th, int start, int total);

void partition_finish(Thread *th);

#endif /* PARTITION_H */
