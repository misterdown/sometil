#include <string.h>
#include <assert.h>

#include "dynamic_string.h"

#define DS_MIN_CAPACITY 16
#define DS_GROWTH_FACTOR 2

static void dstring_ensure_capacity(dstring* ds, size_t required) {
    if (ds->capacity >= required) return;
    
    size_t new_cap = ds->capacity;
    while (new_cap < required) {
        new_cap = new_cap ? new_cap * DS_GROWTH_FACTOR : DS_MIN_CAPACITY;
    }
    
    char* new_data = arena_allocate(ds->arena, new_cap, 1);
    if (ds->size > 0) {
        memcpy(new_data, ds->data, ds->size);
    }
    ds->data = new_data;
    ds->capacity = new_cap;
}

dstring dstring_new(arena_allocator* arena) {
    return (dstring) {
        .arena = arena,
        .data = NULL,
        .capacity = 0,
        .size = 0
    };
}

dstring dstring_from_cstr(arena_allocator* arena, const char* str) {
    dstring ds = dstring_new(arena);
    if (str) {
        size_t len = strlen(str);
        dstring_append_n(&ds, str, len);
    }
    return ds;
}

void dstring_clear(dstring* ds) {
    ds->size = 0;
    if (ds->data && ds->capacity > 0) {
        ds->data[0] = '\0';
    }
}

void dstring_append(dstring* ds, const char* str) {
    if (!str) return;
    dstring_append_n(ds, str, strlen(str));
}

void dstring_append_char(dstring* ds, char c) {
    dstring_ensure_capacity(ds, ds->size + 2);
    ds->data[ds->size++] = c;
    ds->data[ds->size] = '\0';
}

void dstring_append_n(dstring* ds, const char* str, size_t n) {
    if (!str || n == 0) return;
    
    dstring_ensure_capacity(ds, ds->size + n + 1);
    memcpy(ds->data + ds->size, str, n);
    ds->size += n;
    ds->data[ds->size] = '\0';
}

const char* dstring_cstr(const dstring* ds) {
    return ds->data ? ds->data : "";
}

size_t dstring_length(const dstring* ds) {
    return ds->size;
}

bool dstring_empty(const dstring* ds) {
    return ds->size == 0;
}

bool dstring_eq(const dstring* ds1, const dstring* ds2) {
    if (ds1->size != ds2->size) return false;
    return memcmp(ds1->data, ds2->data, ds1->size) == 0;
}

bool dstring_eq_cstr(const dstring* ds, const char* str) {
    if (!str) return ds->size == 0;
    size_t len = strlen(str);
    if (len != ds->size) return false;
    return memcmp(ds->data, str, len) == 0;
}