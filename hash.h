
#ifndef HASH_H
#define HASH_H

#include "defs.h"

/* the first 4 bytes of the hash_table_element are used for the next pointer */
typedef void* hash_table_element;

typedef struct {
  int number_buckets;
  int number_elements;
  hash_table_element *buckets;
} HashTable;

HashTable *hash_table_create(int startSize);
void hash_table_insert(HashTable *hash, hash_table_element elem);
hash_table_element hash_table_get(HashTable *hash, NodeID index);

static inline
int hash_table_total(HashTable *hash)
{
  return hash->number_elements;
}

typedef bool (*HashTableIterateFunction)(hash_table_element, void*);
void hash_table_iterate(HashTable *hash, HashTableIterateFunction fn, void *data);

#endif /* HASH_H */
