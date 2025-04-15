#ifndef __ARENA_ALLOCATOR_H__
#define __ARENA_ALLOCATOR_H__ 1

#include <stddef.h>

typedef struct arena_allocator {
    void* memory_block;
    size_t capacity;
    size_t size;
    struct arena_allocator* next;
} arena_allocator;

typedef struct arena_slice {
    arena_allocator* arena;
    arena_allocator* last_arena;
    size_t saved_size;
    void* allocated;
} arena_slice;

void* arena_allocate(arena_allocator*, size_t size, size_t align);
void arena_drop(arena_allocator*);
arena_slice arena_push(arena_allocator*, size_t size, size_t align);
void arena_pop(arena_slice);
void* arena_realloc(arena_allocator*, void* ptr, size_t old_size, size_t new_size, size_t align);
#endif // __ARENA_ALLOCATOR_H__