#ifndef VECTOR_H
#define VECTOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void *data;
    size_t elem_size;
    size_t size;
    size_t capacity;
} vector_t;

bool vector_init(vector_t *v, size_t elem_size, size_t capacity);
void vector_destroy(vector_t *v);

vector_t *vector_create(size_t elem_size, size_t capacity);
void vector_free(vector_t *v);

bool vector_push(vector_t *v, const void *value);
bool vector_pop(vector_t *v, void *value);

bool vector_insert(vector_t *v, size_t index, const void *value);
bool vector_erase(vector_t *v, size_t index);

typedef bool (*vector_value_cmp)(const void *a, const void *b);
bool vector_find(const vector_t *v, const void *value, vector_value_cmp cmp, size_t *index);
bool vector_get(const vector_t *v, size_t index, void *value);
bool vector_set(vector_t *v, size_t index, const void *value);

size_t vector_size(const vector_t *v);
size_t vector_capacity(const vector_t *v);

bool vector_reserve(vector_t *v, size_t capacity);
void vector_clear(vector_t *v);

#endif
