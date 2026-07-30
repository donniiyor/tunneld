#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "vector.h"
#include <stddef.h>

struct pair {
    void *key;
    size_t key_size;

    void *value;
    size_t value_size;
};

typedef struct {
    vector_t *buckets;
    size_t size;
    size_t buckets_count;
} hashtable_t;

static bool hashtable_pair_cmp(const void *elem, const void *value);

hashtable_t *hashtable_create(size_t bucket_capacity, size_t buckets_count);
void hashtable_destroy(hashtable_t *ht);

bool hashtable_put(hashtable_t *ht, const void *key, const size_t key_size, const void *value, const size_t value_size);
bool hashtable_get(const hashtable_t *ht, const void *key, const size_t key_size, void *value);

bool hashtable_remove(hashtable_t *ht, const void *key, const size_t key_size);

size_t hashtable_size(hashtable_t *ht);

#endif
