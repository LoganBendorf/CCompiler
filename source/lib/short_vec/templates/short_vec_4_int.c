
#include "lib/short_vec/short_vec.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>

extern bool DEBUG;
extern bool TEST;

#define INLINE_CAP 4

typedef size_t vec_size_t;


[[nodiscard]] static vec_size_t get_heap_size(short_vec_4_int* self) {
    if (self->size > INLINE_CAP) {
        return self->size - INLINE_CAP; } 
    return 0;
}
[[nodiscard]] static vec_size_t get_heap_capacity(short_vec_4_int* self) {
    if (self->capacity > INLINE_CAP) {
        return self->capacity - INLINE_CAP; } 
    return 0;
}
[[nodiscard]] static vec_size_t get_inline_size(short_vec_4_int* self) {
    if (self->size < INLINE_CAP) {
        return self->size; } 
    return INLINE_CAP;
}
[[nodiscard]] static int* allocate_heap(const size_t n) { return calloc(n, sizeof(int)); }

void short_vec_4_int_constructor(short_vec_4_int* self) {
    self->heap_array    = NULL;
    self->v_table_index = 0;
    self->size          = 0;
    self->capacity      = INLINE_CAP;
}

void short_vec_4_int_reserve(void* void_self, const size_t new_capacity) {
    short_vec_4_int* self = (short_vec_4_int*) void_self;

    if (self->capacity >= new_capacity) { return; }
    if (new_capacity <= INLINE_CAP) { return; }

    const size_t to_allocate = new_capacity - INLINE_CAP;
    int* new_heap = allocate_heap(to_allocate);
    if (new_heap == NULL) { FATAL_ERROR_STACK_TRACE("FAILED TO ALLOCATE SHORT VEC HEAP"); }
    if (self->heap_array != NULL) {
        memcpy(new_heap, self->heap_array, get_heap_size(self) * sizeof(int));
        free(self->heap_array);
    }
    self->heap_array = new_heap;
    self->capacity   = new_capacity;
}

static void short_vec_4_int_expand(short_vec_4_int* self) {
    const size_t heap_capacity = get_heap_capacity(self);
    if (heap_capacity == 0) {
        short_vec_4_int_reserve(self, INLINE_CAP + 2);
    } else {
        short_vec_4_int_reserve(self, INLINE_CAP + heap_capacity * 2);
    }
}

void short_vec_4_int_insert(void* void_self, const size_t index, const int value) {
    short_vec_4_int* self = (short_vec_4_int*) void_self;

    if (index >= self->size) { FATAL_ERROR_STACK_TRACE("OOB SHORT VEC INSERT INDEX"); }

    if (get_inline_size(self) < INLINE_CAP) {
        self->inline_array[index] = value;
    } else {
        const size_t heap_index = index - INLINE_CAP;
        self->heap_array[heap_index] = value;
    }
}

void short_vec_4_int_push_back(void* void_self, const int value) {
    short_vec_4_int* self = (short_vec_4_int*) void_self;

    const size_t index = self->size;
    if (index < INLINE_CAP) {
        self->inline_array[index] = value;
    } else {
        const size_t heap_index    = index - INLINE_CAP;
        const size_t heap_capacity = get_heap_capacity(self);
        if (heap_index >= heap_capacity) {
            short_vec_4_int_expand(self);
        }
        self->heap_array[index - INLINE_CAP] = value;
    }
    self->size += 1;
}

void short_vec_4_int_clear(void* void_self) {
    short_vec_4_int* self = (short_vec_4_int*) void_self;

    if (self->heap_array != NULL) { free(self->heap_array); }
    self->heap_array = NULL;
    self->size       = 0;
    self->capacity   = INLINE_CAP;
}

int short_vec_4_int_index(void* void_self, const size_t index) {
    short_vec_4_int* self = (short_vec_4_int*) void_self;

    if (index < INLINE_CAP) {
        return self->inline_array[index];
    } else {
        const size_t heap_index = index - INLINE_CAP;
        if (heap_index >= get_heap_size(self)) { FATAL_ERROR_STACK_TRACE("OOB SHORT VEC INDEX"); }
        return self->heap_array[heap_index];
    }
}

void short_vec_4_int_destroy(void* void_self) {
    short_vec_4_int* self = (short_vec_4_int*) void_self;

    int* heap_array = self->heap_array;
    if (heap_array != NULL) { free(heap_array); }
}


