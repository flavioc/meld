
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "utils.h"
#include "hash.h"

#define BUCKET_THRESHOLD 4
#define HASH_INDEX(size, index) ((unsigned int)(index - 1) & (size - 1))
#define INDEX_FIELD(ELEM) (*(NodeID*)((hash_table_element*)(ELEM)+1))
#define NEXT_FIELD(ELEM) (*(hash_table_element*)(ELEM))

static int
next_power_of_two(int num)
{
  --num;
  
  num = (num >> 1) | num;
  num = (num >> 2) | num;
  num = (num >> 4) | num;
  num = (num >> 8) | num;
  num = (num >> 16) | num;
  
  return ++num;
}

HashTable*
hash_table_create(int start_size)
{
  int real_size = next_power_of_two(start_size);
  HashTable *hash = (HashTable*)meld_malloc(sizeof(HashTable));
  
  hash->number_buckets = real_size;
  hash->number_elements = 0;
  
  /* create buckets */
  hash->buckets = (hash_table_element*)meld_calloc(hash->number_buckets,
    sizeof(hash_table_element));
    
  return hash;
}

void
hash_table_insert(HashTable *hash, hash_table_element elem)
{
  NodeID index = HASH_INDEX(hash->number_buckets, INDEX_FIELD(elem));
  hash_table_element *bucket = hash->buckets + index;
  int total = 1;
  
  while (NEXT_FIELD(bucket)) {
    ++total;

    bucket = NEXT_FIELD(bucket);
  }
  
  NEXT_FIELD(bucket) = elem;
  NEXT_FIELD(elem) = NULL;
  hash->number_elements++;
  
  if (total > BUCKET_THRESHOLD) {
    /* must expand hash table */
    int next_size = hash->number_buckets << 1;
    hash_table_element *new_buckets = (hash_table_element*)meld_calloc(next_size,
            sizeof(hash_table_element));
    
    /* reinsert everything again */
    int i;
    for(i = 0; i < hash->number_buckets; ++i) {
      hash_table_element *iter_bucket = NEXT_FIELD(hash->buckets + i);
      
      while (iter_bucket) {
        /* insert this element */

        hash_table_element elem = (hash_table_element)iter_bucket;
        hash_table_element *next = NEXT_FIELD(iter_bucket);
        
        index = HASH_INDEX(next_size, INDEX_FIELD(elem));
        hash_table_element *bucket = new_buckets + index;
        hash_table_element old = NEXT_FIELD(bucket);

        NEXT_FIELD(bucket) = elem;
        NEXT_FIELD(elem) = old;
        
        iter_bucket = next;
      }
    }
    
    meld_free(hash->buckets);
    
    hash->buckets = new_buckets;
    hash->number_buckets = next_size;
  }
}

hash_table_element
hash_table_get(HashTable *hash, NodeID index)
{
  NodeID hash_index = HASH_INDEX(hash->number_buckets, index);
  hash_table_element *bucket = NEXT_FIELD(hash->buckets + hash_index);
  
  while (bucket) {
    if (INDEX_FIELD(bucket) == index)
      return bucket;
    
    bucket = NEXT_FIELD(bucket);
  }
  
  return NULL;
}

void
hash_table_iterate(HashTable *hash, HashTableIterateFunction fn, void *data)
{
	int i;
	for(i = 0; i < hash->number_buckets; ++i) {
		hash_table_element *iterBucket = NEXT_FIELD(hash->buckets + i);

		while (iterBucket) {
        /* insert this element */

        hash_table_element elem = (hash_table_element)iterBucket;

				if(!fn(elem, data))
					return; /* return if fn returned false */

        iterBucket = NEXT_FIELD(iterBucket);
		}
	}
}
