
import os


TYPE_INFO = [
    ("int",    4),
    ("double", 4),
    ("char",   4),
]


c_template = """
#include "lib/short_vec/short_vec.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>

extern bool DEBUG;
extern bool TEST;

#define INLINE_CAP {INLINE_CAP}

typedef {VEC_SIZE_T} vec_size_t;


[[nodiscard]] static vec_size_t get_heap_size(short_vec_{INLINE_CAP}_{T}* self) {{
    if (self->size > INLINE_CAP) {{
        return self->size - INLINE_CAP; }} 
    return 0;
}}
[[nodiscard]] static vec_size_t get_heap_capacity(short_vec_{INLINE_CAP}_{T}* self) {{
    if (self->capacity > INLINE_CAP) {{
        return self->capacity - INLINE_CAP; }} 
    return 0;
}}
[[nodiscard]] static vec_size_t get_inline_size(short_vec_{INLINE_CAP}_{T}* self) {{
    if (self->size < INLINE_CAP) {{
        return self->size; }} 
    return INLINE_CAP;
}}
[[nodiscard]] static {T}* allocate_heap(const size_t n) {{ return calloc(n, sizeof({T})); }}

void short_vec_{INLINE_CAP}_{T}_constructor(short_vec_{INLINE_CAP}_{T}* self) {{
    self->heap_array    = NULL;
    self->v_table_index = {i};
    self->size          = 0;
    self->capacity      = INLINE_CAP;
}}

void short_vec_{INLINE_CAP}_{T}_reserve(void* void_self, const size_t new_capacity) {{
    short_vec_{INLINE_CAP}_{T}* self = (short_vec_{INLINE_CAP}_{T}*) void_self;

    if (self->capacity >= new_capacity) {{ return; }}
    if (new_capacity <= INLINE_CAP) {{ return; }}

    const size_t to_allocate = new_capacity - INLINE_CAP;
    {T}* new_heap = allocate_heap(to_allocate);
    if (new_heap == NULL) {{ FATAL_ERROR_STACK_TRACE("FAILED TO ALLOCATE SHORT VEC HEAP"); }}
    if (self->heap_array != NULL) {{
        memcpy(new_heap, self->heap_array, get_heap_size(self) * sizeof({T}));
        free(self->heap_array);
    }}
    self->heap_array = new_heap;
    self->capacity   = new_capacity;
}}

static void short_vec_{INLINE_CAP}_{T}_expand(short_vec_{INLINE_CAP}_{T}* self) {{
    const size_t heap_capacity = get_heap_capacity(self);
    if (heap_capacity == 0) {{
        short_vec_{INLINE_CAP}_{T}_reserve(self, INLINE_CAP + 2);
    }} else {{
        short_vec_{INLINE_CAP}_{T}_reserve(self, INLINE_CAP + heap_capacity * 2);
    }}
}}

void short_vec_{INLINE_CAP}_{T}_insert(void* void_self, const size_t index, const {T} value) {{
    short_vec_{INLINE_CAP}_{T}* self = (short_vec_{INLINE_CAP}_{T}*) void_self;

    if (index >= self->size) {{ FATAL_ERROR_STACK_TRACE("OOB SHORT VEC INSERT INDEX"); }}

    if (get_inline_size(self) < INLINE_CAP) {{
        self->inline_array[index] = value;
    }} else {{
        const size_t heap_index = index - INLINE_CAP;
        self->heap_array[heap_index] = value;
    }}
}}

void short_vec_{INLINE_CAP}_{T}_push_back(void* void_self, const {T} value) {{
    short_vec_{INLINE_CAP}_{T}* self = (short_vec_{INLINE_CAP}_{T}*) void_self;

    const size_t index = self->size;
    if (index < INLINE_CAP) {{
        self->inline_array[index] = value;
    }} else {{
        const size_t heap_index    = index - INLINE_CAP;
        const size_t heap_capacity = get_heap_capacity(self);
        if (heap_index >= heap_capacity) {{
            short_vec_{INLINE_CAP}_{T}_expand(self);
        }}
        self->heap_array[index - INLINE_CAP] = value;
    }}
    self->size += 1;
}}

void short_vec_{INLINE_CAP}_{T}_clear(void* void_self) {{
    short_vec_{INLINE_CAP}_{T}* self = (short_vec_{INLINE_CAP}_{T}*) void_self;

    if (self->heap_array != NULL) {{ free(self->heap_array); }}
    self->heap_array = NULL;
    self->size       = 0;
    self->capacity   = INLINE_CAP;
}}

{T} short_vec_{INLINE_CAP}_{T}_index(void* void_self, const size_t index) {{
    short_vec_{INLINE_CAP}_{T}* self = (short_vec_{INLINE_CAP}_{T}*) void_self;

    if (index < INLINE_CAP) {{
        return self->inline_array[index];
    }} else {{
        const size_t heap_index = index - INLINE_CAP;
        if (heap_index >= get_heap_size(self)) {{ FATAL_ERROR_STACK_TRACE("OOB SHORT VEC INDEX"); }}
        return self->heap_array[heap_index];
    }}
}}

void short_vec_{INLINE_CAP}_{T}_destroy(void* void_self) {{
    short_vec_{INLINE_CAP}_{T}* self = (short_vec_{INLINE_CAP}_{T}*) void_self;

    {T}* heap_array = self->heap_array;
    if (heap_array != NULL) {{ free(heap_array); }}
}}

"""

header_template = """
typedef struct short_vec_{INLINE_CAP}_{T} {{
    {T}* heap_array;
    unsigned int v_table_index;
    {VEC_SIZE_T} size;
    {VEC_SIZE_T} capacity;
    {T} inline_array[{INLINE_CAP}]; 
}} short_vec_{INLINE_CAP}_{T};
void short_vec_{INLINE_CAP}_{T}_constructor(short_vec_{INLINE_CAP}_{T}* self);
void short_vec_{INLINE_CAP}_{T}_reserve    (void* void_self, const size_t new_capacity);
void short_vec_{INLINE_CAP}_{T}_insert     (void* void_self, const size_t index, const {T} elem);
void short_vec_{INLINE_CAP}_{T}_push_back  (void* void_self, const {T} elem);
void short_vec_{INLINE_CAP}_{T}_clear      (void* void_self);
void short_vec_{INLINE_CAP}_{T}_destroy    (void* void_self);
[[nodiscard]] {T} short_vec_{INLINE_CAP}_{T}_index (void* void_self, const size_t index);
"""

vtable_template = """
    case {i}:
        switch (operation_index) {{
        case 0:
            return (void (*)(void)) &short_vec_{INLINE_CAP}_{T}_reserve;
        case 1:
            return (void (*)(void)) &short_vec_{INLINE_CAP}_{T}_insert;
        case 2:
            return (void (*)(void)) &short_vec_{INLINE_CAP}_{T}_push_back;
        case 3:
            return (void (*)(void)) &short_vec_{INLINE_CAP}_{T}_clear;
        case 4:
            return (void (*)(void)) &short_vec_{INLINE_CAP}_{T}_destroy;
        case 5:
            return (void (*)(void)) &short_vec_{INLINE_CAP}_{T}_index;
        }}"""

location = "source/lib/short_vec/"
header_path = location + "short_vec.h"
vtable_path = location + "short_vec_v_table.c"


# Clear junk
if os.path.exists(header_path):
    os.remove(header_path)
if os.path.exists(vtable_path):
    os.remove(vtable_path)



# Header
with open(header_path, "w") as f:
    print("#pragma once", file=f)
    print("#include <stddef.h>\n", file=f)
    print("void (*short_vec_v_table(const size_t type_index, const size_t operation_index))(void);", file=f)
    def_str = """
#define S_VEC_RESERVE(vec, capacity) ((void (*)(void*, size_t)) short_vec_v_table((vec).v_table_index, 0)) (&(vec), capacity)
#define S_VEC_INSERT(vec, index, value) \\
    _Generic((vec).inline_array[0], \\
        int:    ((void (*)(void*, size_t, int))    short_vec_v_table((vec).v_table_index, 1))(&(vec), index, value), \\
        double: ((void (*)(void*, size_t, double)) short_vec_v_table((vec).v_table_index, 1))(&(vec), index, value), \\
        char:   ((void (*)(void*, size_t, char))   short_vec_v_table((vec).v_table_index, 1))(&(vec), index, value) \\
    )
#define S_VEC_PUSH_BACK(vec, value) \\
    _Generic((vec).inline_array[0], \\
        int:    ((void (*)(void*, int))    short_vec_v_table((vec).v_table_index, 2))(&(vec), value), \\
        double: ((void (*)(void*, double)) short_vec_v_table((vec).v_table_index, 2))(&(vec), value), \\
        char:   ((void (*)(void*, char))   short_vec_v_table((vec).v_table_index, 2))(&(vec), value) \\
    )
#define S_VEC_CLEAR(vec)   ((void (*)(void*)) short_vec_v_table((vec).v_table_index, 3)) (&(vec))
#define S_VEC_DESTROY(vec) ((void (*)(void*)) short_vec_v_table((vec).v_table_index, 4)) (&(vec))
#define S_VEC_INDEX(vec, index) \\
    _Generic((vec).inline_array[0], \\
        int:    ((int    (*)(void*, size_t)) short_vec_v_table((vec).v_table_index, 5)), \\
        double: ((double (*)(void*, size_t)) short_vec_v_table((vec).v_table_index, 5)), \\
        char:   ((char   (*)(void*, size_t)) short_vec_v_table((vec).v_table_index, 5)) \\
    )(&(vec), index)
"""
    print(def_str, file=f)

# V table
with open(vtable_path, "a") as f:
    print(
    """
#include <stddef.h>
#include "short_vec.h"

void (*short_vec_v_table(const size_t type_index, const size_t operation_index))(void) {
    switch (type_index) {"""
    , file=f)

vec_size_t = "size_t"
i = 0
for T, INLINE_CAP in TYPE_INFO:
    # C files
    with open(location + "templates/short_vec_" + str(INLINE_CAP) + "_" + T + ".c", "w") as f:
        print(c_template.format(VEC_SIZE_T=vec_size_t, T=T, INLINE_CAP=INLINE_CAP, i=i), file=f)
    
    # V table
    with open(vtable_path, "a") as f:
        print(vtable_template.format(T=T, INLINE_CAP=INLINE_CAP, i=i), file=f)

    # Header
    with open(header_path, "a") as f:
        print(header_template.format(VEC_SIZE_T=vec_size_t, T=T, INLINE_CAP=INLINE_CAP), file=f)

    i += 1


# V table end
with open(vtable_path, "a") as f:
        print(
        "   }\n"
        "\n   return NULL;\n"
        "}\n"
        , file=f)