#include "vector.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

bool vector_init(vector_t *v, size_t elem_size, size_t capacity) {
    assert(v != NULL);

    vector_t *tmp = vector_create(elem_size, capacity);
    if (tmp == NULL) return false;

    *v = *tmp;
    free(tmp);

    return true;
}

void vector_destroy(vector_t *v) {
    assert(v != NULL);

    free(v->data);
    v->data = NULL;
    v->size = 0;
    v->capacity = 0;
}

vector_t *vector_create(size_t elem_size, size_t capacity) {
    vector_t *v = malloc(sizeof(vector_t));
    if (v == NULL) return NULL;

    v->data = malloc(elem_size * capacity);
    if (v->data == NULL) {
        free(v);
        return NULL;
    }

    v->elem_size = elem_size;
    v->size = 0;
    v->capacity = capacity;

    return v;
}

void vector_free(vector_t *v) {
    assert(v != NULL);

    vector_destroy(v);
    free(v);
}

bool vector_push(vector_t *v, const void *value) {
    assert(v != NULL);

    if (v->size == v->capacity) {
        size_t new_capacity = v->capacity * 2;
        if (!vector_reserve(v, new_capacity)) return false;
    }

    memcpy((char *)v->data + v->size * v->elem_size, value, v->elem_size);

    v->size++;

    return true;
}

bool vector_pop(vector_t *v, void *value) {
    assert(v != NULL);

    if (v->size == 0) return false;

    v->size--;

    if (value != NULL) memcpy(value, (char *)v->data + v->size * v->elem_size, v->elem_size);

    return true;
}

bool vector_insert(vector_t *v, size_t index, const void *value) {
    assert(v != NULL);

    if (index > v->size) return false;

    if (v->size == v->capacity) {
        if (!vector_reserve(v, v->capacity * 2)) return false;
    }

    char *base = v->data;

    memmove(base + (index + 1) * v->elem_size, base + index * v->elem_size, (v->size - index) * v->elem_size);
    memcpy(base + index * v->elem_size, value, v->elem_size);

    v->size++;

    return true;
}

bool vector_erase(vector_t *v, size_t index) {
    assert(v != NULL);

    if (index >= v->size) return false;

    char *base = v->data;

    memmove(base + index * v->elem_size, base + (index + 1) * v->elem_size, (v->size - index - 1) * v->elem_size);

    v->size--;

    return true;
}

bool vector_find(const vector_t *v, const void *value, vector_value_cmp cmp, size_t *index) {
    assert(v != NULL && cmp != NULL);

    const char *base = v->data;

    for (size_t i = 0; i < v->size; i++) {
        const void *elem = base + i * v->elem_size;

        if (cmp(elem, value)) {
            *index = i;

            return true;
        }
    }

    return false;
}

bool vector_get(const vector_t *v, size_t index, void *value) {
    assert(v != NULL);

    if (index >= v->size) return false;

    memcpy(value, (char *)v->data + index * v->elem_size, v->elem_size);

    return true;
}

bool vector_set(vector_t *v, size_t index, const void *value) {
    assert(v != NULL);

    if (index >= v->size) return false;

    memcpy((char *)v->data + index * v->elem_size, value, v->elem_size);

    return true;
}

size_t vector_size(const vector_t *v) {
    assert(v != NULL);

    return v->size;
}

size_t vector_capacity(const vector_t *v) {
    assert(v != NULL);

    return v->capacity;
}

bool vector_reserve(vector_t *v, size_t capacity) {
    assert(v != NULL);

    void *tmp_data = realloc(v->data, v->elem_size * capacity);
    if (tmp_data == NULL) return false;

    v->data = tmp_data;
    v->capacity = capacity;

    return true;
}

void vector_clear(vector_t *v) {
    assert(v != NULL);

    v->size = 0;
}
