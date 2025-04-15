#include <stdalign.h>
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arena_allocator.h"


#define ALIGN_UP(ptr, align) (((size_t)(ptr) + ((align) - 1)) & ~((align) - 1))
#define GENERAL_ALLOC(size) calloc(1, size)
#define GENERAL_FREE(ptr) free(ptr)

void* arena_allocate(arena_allocator* arr, size_t size, size_t align) {
    assert(arr != NULL);
    assert(size != 0);
    assert(align != 0 && (align & (align - 1)) == 0);
    
    if (arr->memory_block == NULL) {
        size_t block_size = (size > 1024) ? size * 2 : 1024;
        void* allocated = GENERAL_ALLOC(block_size);
        assert(allocated != NULL && "Failed to allocate memory block");
        
        arr->memory_block = allocated;
        arr->capacity = block_size;
        arr->size = 0;
        arr->next = NULL;
    }

    void* current_ptr = (char*)arr->memory_block + arr->size;
    void* aligned_ptr = (void*)ALIGN_UP((size_t)current_ptr, align);
    size_t actual_size = size + ((char*)aligned_ptr - (char*)current_ptr);

    if (arr->size + actual_size <= arr->capacity) {
        arr->size += actual_size;
        return aligned_ptr;
    }

    if (arr->next == NULL) {
        arena_allocator* next_arena = (arena_allocator*)GENERAL_ALLOC(sizeof(arena_allocator));
        assert(next_arena != NULL && "Failed to allocate new arena");
        memset(next_arena, 0, sizeof(arena_allocator));
        arr->next = next_arena;
    }

    return arena_allocate(arr->next, size, align);
}

void arena_drop(arena_allocator* arr) {
    if (arr == NULL) return;
    
    arena_allocator* current = arr;
    while (current != NULL) {
        arena_allocator* next = current->next;
        if (current->memory_block != NULL) {
            GENERAL_FREE(current->memory_block);
            current->memory_block = NULL;
        }
        if (current != arr) {
            GENERAL_FREE(current);
        }
        current = next;
    }
    arr->next = NULL;
    arr->size = 0;
    arr->capacity = 0;
}

arena_slice arena_push(arena_allocator* arr, size_t size, size_t align) {
    assert(arr != NULL);
    
    arena_allocator* last_arena = arr;
    while (last_arena->next != NULL) {
        last_arena = last_arena->next;
    }
    
    arena_slice slice = {
        .arena = arr,
        .last_arena = last_arena,
        .saved_size = last_arena->size,
        .allocated = arena_allocate(arr, size, align),
    };
    
    return slice;
}

void arena_pop(arena_slice slice) {
    assert(slice.arena != NULL);
    
    slice.last_arena->size = slice.saved_size;
    
    arena_allocator* current = slice.last_arena->next;
    while (current != NULL) {
        arena_allocator* next = current->next;
        if (current->memory_block != NULL) {
            GENERAL_FREE(current->memory_block);
        }
        GENERAL_FREE(current);
        current = next;
    }
    slice.last_arena->next = NULL;
}

void* arena_realloc(arena_allocator* arr, void* ptr, size_t old_size, size_t new_size, size_t align) {
    assert(arr != NULL);
    assert(align != 0 && (align & (align - 1)) == 0);
    
    // Act like arena_allocate
    if (ptr == NULL) {
        return arena_allocate(arr, new_size, align);
    }
    
    // Act like arena_free(does not exist, soo. . .)
    if (new_size == 0) {
        return NULL;
    }
    
    arena_allocator* current = arr;
    while (current != NULL) {
        if (ptr >= current->memory_block && 
            (char*)ptr < (char*)current->memory_block + current->capacity) {
            break;
        }
        current = current->next;
    }
    if (current == NULL) {
        assert(0 && "Pointer does not belong to any arena");
        return NULL;
    }

    // Just shift
    if ((char*)ptr + new_size <= (char*)current->memory_block + current->capacity) {
        current->size += (new_size - old_size);
        return ptr;
    }
    
    // Not enough capacity - allocate new
    void* new_ptr = arena_allocate(arr, new_size, align);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    // Copy to new allocated block
    memcpy(new_ptr, ptr, old_size > new_size ? new_size : old_size);
    
    return new_ptr;
}