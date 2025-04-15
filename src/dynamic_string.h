#ifndef __DYNAMIC_STRING_H__
#define __DYNAMIC_STRING_H__ 1

// No SSO

#include <stdbool.h>
#include <stddef.h>

#include "arena_allocator.h"

typedef struct dstring {
    arena_allocator* arena;
    char* data;
    size_t capacity;
    size_t size;
} dstring;

dstring dstring_new(arena_allocator* arena);
dstring dstring_from_cstr(arena_allocator* arena, const char* str);

void dstring_clear(dstring* ds);

void dstring_append(dstring* ds, const char* str);
void dstring_append_char(dstring* ds, char c);
void dstring_append_n(dstring* ds, const char* str, size_t n);

const char* dstring_cstr(const dstring* ds);
size_t dstring_length(const dstring* ds);
bool dstring_empty(const dstring* ds);

bool dstring_eq(const dstring* ds1, const dstring* ds2);
bool dstring_eq_cstr(const dstring* ds, const char* str);

#endif // __DYNAMIC_STRING_H__