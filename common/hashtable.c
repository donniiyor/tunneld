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
        vector_t *bucket = &ht->buckets[i];

        for (size_t j = 0; j < bucket->size; j++) {
            struct pair p;
            vector_get(bucket, j, &p);

            free(p.key);
            free(p.value);
        }

        vector_destroy(bucket);
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

    for (size_t i = 0; i < bucket->size; i++) {
        struct pair p;
        vector_get(bucket, i, &p);

        if (p.key_size == key_size && memcmp(p.key, key, key_size) == 0) {
            void *value_copy = malloc(value_size);
            if (value_copy == NULL) return false;

            memcpy(value_copy, value, value_size);
            free(p.value);
            p.value = value_copy;
            p.value_size = value_size;

            return vector_set(bucket, i, &p);
        }
    }

    void *key_copy = malloc(key_size);
    if (key_copy == NULL) return false;

    void *value_copy = malloc(value_size);
    if (value_copy == NULL) {
        free(key_copy);
        return false;
    }

    memcpy(key_copy, key, key_size);
    memcpy(value_copy, value, value_size);

    struct pair p = {.key = key_copy, .key_size = key_size, .value = value_copy, .value_size = value_size};

    if (!vector_push(bucket, &p)) {
        free(key_copy);
        free(value_copy);
        return false;
    }

    ht->size++;

    return true;
}

static bool hashtable_pair_cmp(const void *elem, const void *value) {
    const struct pair *pair = elem;
    const struct pair *needle = value;

    return pair->key_size == needle->key_size && memcmp(pair->key, needle->key, pair->key_size) == 0;
}

bool hashtable_get(const hashtable_t *ht, const void *key, const size_t key_size, void *value) {
    assert(ht != NULL);

    size_t bucket_index = hash(key, key_size) % ht->buckets_count;
    vector_t *bucket = &ht->buckets[bucket_index];
    struct pair needle = {.key = (void *)key, .key_size = key_size};

    size_t elem_index;
    if (!vector_find(bucket, &needle, hashtable_pair_cmp, &elem_index)) return false;

    struct pair elem;
    if (!vector_get(bucket, elem_index, &elem)) return false;

    memcpy(value, elem.value, elem.value_size);

    return true;
}

bool hashtable_remove(hashtable_t *ht, const void *key, const size_t key_size) {
    assert(ht != NULL);

    size_t bucket_index = hash(key, key_size) % ht->buckets_count;
    vector_t *bucket = &ht->buckets[bucket_index];
    struct pair needle = {.key = (void *)key, .key_size = key_size};

    size_t elem_index;
    if (!vector_find(bucket, &needle, hashtable_pair_cmp, &elem_index)) return false;

    struct pair elem;
    if (!vector_get(bucket, elem_index, &elem)) return false;

    free(elem.key);
    free(elem.value);

    if (!vector_erase(bucket, elem_index)) return false;

    ht->size--;

    return true;
}

size_t hashtable_size(hashtable_t *ht) {
    assert(ht != NULL);

    return ht->size;
}
