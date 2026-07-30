#include "hashtable.h"
#include "vector.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

hashtable_t *hashtable_create(size_t bucket_capacity, size_t buckets_count) {
    hashtable_t *ht = malloc(sizeof(hashtable_t));
    if (ht == NULL) return NULL;

    ht->buckets = calloc(buckets_count, sizeof(vector_t));
    if (ht->buckets == NULL) {
        free(ht);
        return NULL;
    }

    for (size_t i = 0; i < buckets_count; i++) {
        vector_init(&ht->buckets[i], sizeof(struct pair), bucket_capacity);
    }

    ht->buckets_count = buckets_count;
    ht->size = 0;

    return ht;
}

void hashtable_destroy(hashtable_t *ht) {
    assert(ht != NULL);

    for (size_t i = 0; i < ht->buckets_count; i++) {
        vector_destroy(&ht->buckets[i]);
    }

    free(ht->buckets);
    free(ht);
}

static uint64_t hash(const void *key, size_t key_size) {
    const uint8_t *bytes = key;

    uint64_t h = 14695981039346656037ULL;

    for (size_t i = 0; i < key_size; i++) {
        h ^= bytes[i];
        h *= 1099511628211ULL;
    }

    return h;
}

bool hashtable_put(hashtable_t *ht, const void *key, const size_t key_size, const void *value,
                   const size_t value_size) {
    assert(ht != NULL);

    size_t index = hash(key, key_size) % ht->buckets_count;
    vector_t *bucket = &ht->buckets[index];

    int item_index = -1;

    for (size_t i = 0; i < bucket->size; i++) {
        struct pair p;
        vector_get(bucket, i, (void *)&p);

        if (strcmp(p.key, key) == 0) {
            item_index = i;
            break;
        }
    }

    struct pair p = {.key = (char *)key, .value = (void *)value, .value_size = value_size};

    if (item_index == -1) {
        vector_push(bucket, &p);
        ht->size++;
    } else vector_set(bucket, item_index, &p);

    return true;
}

static bool hashtable_pair_cmp(const void *elem, const void *value) {
    const struct pair *pair = elem;
    const void *key = value;

    return strcmp(pair->key, key) == 0;
}

bool hashtable_get(const hashtable_t *ht, const void *key, const size_t key_size, void *value) {
    assert(ht != NULL);

    size_t bucket_index = hash(key, key_size) % ht->buckets_count;
    vector_t *bucket = &ht->buckets[bucket_index];

    size_t elem_index;
    if (!vector_find(bucket, key, hashtable_pair_cmp, &elem_index)) return false;

    struct pair elem;
    if (!vector_get(bucket, elem_index, &elem)) return false;

    memcpy(value, elem.value, elem.value_size);

    return true;
}

bool hashtable_remove(hashtable_t *ht, const void *key, const size_t key_size) {
    assert(ht != NULL);

    size_t bucket_index = hash(key, key_size) % ht->buckets_count;
    vector_t *bucket = &ht->buckets[bucket_index];

    size_t elem_index;
    if (!vector_find(bucket, key, hashtable_pair_cmp, &elem_index)) return false;

    if (!vector_erase(bucket, elem_index)) return false;

    ht->size--;

    return true;
}

size_t hashtable_size(hashtable_t *ht) {
    assert(ht != NULL);

    return ht->size;
}
