#pragma once
#include <stddef.h>

void (*short_vec_v_table(const size_t type_index, const size_t operation_index))(void);

#define S_VEC_RESERVE(vec, capacity) ((void (*)(void*, size_t)) short_vec_v_table((vec).v_table_index, 0)) (&(vec), capacity)
#define S_VEC_INSERT(vec, index, value) \
    _Generic((vec).inline_array[0], \
        int:    ((void (*)(void*, size_t, int))    short_vec_v_table((vec).v_table_index, 1))(&(vec), index, value), \
        double: ((void (*)(void*, size_t, double)) short_vec_v_table((vec).v_table_index, 1))(&(vec), index, value), \
        char:   ((void (*)(void*, size_t, char))   short_vec_v_table((vec).v_table_index, 1))(&(vec), index, value) \
    )
#define S_VEC_PUSH_BACK(vec, value) \
    _Generic((vec).inline_array[0], \
        int:    ((void (*)(void*, int))    short_vec_v_table((vec).v_table_index, 2))(&(vec), value), \
        double: ((void (*)(void*, double)) short_vec_v_table((vec).v_table_index, 2))(&(vec), value), \
        char:   ((void (*)(void*, char))   short_vec_v_table((vec).v_table_index, 2))(&(vec), value) \
    )
#define S_VEC_CLEAR(vec)   ((void (*)(void*)) short_vec_v_table((vec).v_table_index, 3)) (&(vec))
#define S_VEC_DESTROY(vec) ((void (*)(void*)) short_vec_v_table((vec).v_table_index, 4)) (&(vec))
#define S_VEC_INDEX(vec, index) \
    _Generic((vec).inline_array[0], \
        int:    ((int    (*)(void*, size_t)) short_vec_v_table((vec).v_table_index, 5)), \
        double: ((double (*)(void*, size_t)) short_vec_v_table((vec).v_table_index, 5)), \
        char:   ((char   (*)(void*, size_t)) short_vec_v_table((vec).v_table_index, 5)) \
    )(&(vec), index)


typedef struct short_vec_4_int {
    int* heap_array;
    unsigned int v_table_index;
    size_t size;
    size_t capacity;
    int inline_array[4]; 
} short_vec_4_int;
void short_vec_4_int_constructor(short_vec_4_int* self);
void short_vec_4_int_reserve    (void* void_self, const size_t new_capacity);
void short_vec_4_int_insert     (void* void_self, const size_t index, const int elem);
void short_vec_4_int_push_back  (void* void_self, const int elem);
void short_vec_4_int_clear      (void* void_self);
void short_vec_4_int_destroy    (void* void_self);
[[nodiscard]] int short_vec_4_int_index (void* void_self, const size_t index);


typedef struct short_vec_4_double {
    double* heap_array;
    unsigned int v_table_index;
    size_t size;
    size_t capacity;
    double inline_array[4]; 
} short_vec_4_double;
void short_vec_4_double_constructor(short_vec_4_double* self);
void short_vec_4_double_reserve    (void* void_self, const size_t new_capacity);
void short_vec_4_double_insert     (void* void_self, const size_t index, const double elem);
void short_vec_4_double_push_back  (void* void_self, const double elem);
void short_vec_4_double_clear      (void* void_self);
void short_vec_4_double_destroy    (void* void_self);
[[nodiscard]] double short_vec_4_double_index (void* void_self, const size_t index);


typedef struct short_vec_4_char {
    char* heap_array;
    unsigned int v_table_index;
    size_t size;
    size_t capacity;
    char inline_array[4]; 
} short_vec_4_char;
void short_vec_4_char_constructor(short_vec_4_char* self);
void short_vec_4_char_reserve    (void* void_self, const size_t new_capacity);
void short_vec_4_char_insert     (void* void_self, const size_t index, const char elem);
void short_vec_4_char_push_back  (void* void_self, const char elem);
void short_vec_4_char_clear      (void* void_self);
void short_vec_4_char_destroy    (void* void_self);
[[nodiscard]] char short_vec_4_char_index (void* void_self, const size_t index);

