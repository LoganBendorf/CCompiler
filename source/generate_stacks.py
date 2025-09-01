
TYPE_INFO = [
    ("asm_node", "ASM_NODE", "source/codegen/asm_node_mem"),
    ("tac_node", "TAC_NODE", "source/tac/tac_node_mem"),
    ("node",     "NODE",     "source/ast/node_mem"),
]


c_template = """
#include "{name}_mem.h"
#include "{name}_size.h"
#include "logger.h"
#include <string.h>
#include <stdio.h>


string get_{name}_string(const size_t index);


extern bool DEBUG;
extern bool TEST;


static size_t prev_stack_push_size = 0;


{name}_stack g_{name}_stack = {{0, {{0}}}};


[[nodiscard]] static size_t size_of_{name}_at_ptr(const {name}_ptr ptr) {{ return size_of_{name}(({name}_type) *ptr.addr); }}

[[nodiscard]] static {name}_stack_ptr g_{name}_stack_top_ptr() {{ return ({name}_stack_ptr){{&g_{name}_stack.stack[g_{name}_stack.top], false}}; }}


{name}_stack_ptr get_top_of_g_{name}_stack() {{
    if (g_{name}_stack.top == 0) {{ return ({name}_stack_ptr){{.addr=0, .is_none=true}}; }}

    const size_t size = prev_stack_push_size;
    if (size > g_{name}_stack.top) {{
        const uint8_t* top_addr = g_{name}_stack_top_ptr().addr;
        const {name}_type top_type = *top_addr;
        
        FATAL_ERROR_STACK_TRACE("Can not get top if global {name} stack, would lead to underflow.\\nTop = %zu, request size = %zu. Top type = (%.*s)", 
                                    g_{name}_stack.top, size,
                                    (int) get_{name}_string(top_type).size, get_{name}_string(top_type).data); 
    }}

    {name}_stack_ptr ret = g_{name}_stack_top_ptr();
    ret.addr -= size;

    return ret;
}}

void push_g_{name}_stack(const {name}_ptr ptr) {{
    if (ptr.addr == NULL) {{ FATAL_ERROR_STACK_TRACE("Can't push NULL pointer"); }}

    const size_t size = size_of_{name}_at_ptr(ptr);
    if (size + g_{name}_stack.top >= {capital_name}_STACK_SIZE) {{
        FATAL_ERROR_STACK_TRACE("Stack OOM"); }}

    memcpy((void*) g_{name}_stack_top_ptr().addr,  (void*) ptr.addr, size);
    
    g_{name}_stack.top += size;

    prev_stack_push_size = size;
}}



const uint8_t* {name}_stack_iter_next({name}_stack_iter* self) {{
    if (self->stack_pos >= {capital_name}_STACK_SIZE) {{ return NULL; }}

    const uint8_t* addr = g_{name}_stack.stack + self->stack_pos;
    const size_t jump_size = size_of_{name}(({name}_type) *(addr));

    self->stack_pos += jump_size;

    return addr;
}}

void {name}_stack_iter_constructor({name}_stack_iter* self) {{
    self->stack_pos = 0;
    self->next = &{name}_stack_iter_next;
}}

"""

header_template = """
#pragma once
#include "{name}.h"
#include <stdint.h>
#include <stddef.h>


#define {capital_name}_STACK_SIZE 1024 * 16


typedef struct {{
    size_t  top;
    uint8_t stack[{capital_name}_STACK_SIZE];
}} {name}_stack;

typedef struct {{
    uint8_t*   addr;
    const bool is_none;
}} {name}_stack_ptr;


[[nodiscard]] {name}_stack_ptr get_top_of_g_{name}_stack();
[[nodiscard]] {name}_stack_ptr place_on_{name}_stack(uint8_t* {name}_addr);

void push_g_{name}_stack(const {name}_ptr ptr);


typedef struct {name}_stack_iter {{
    size_t stack_pos;
    const uint8_t* (*next)(struct {name}_stack_iter* self);
}} {name}_stack_iter;

[[nodiscard]] const uint8_t* {name}_stack_iter_next({name}_stack_iter* self);

void {name}_stack_iter_constructor({name}_stack_iter* self);

"""


for name, capital_name, location in TYPE_INFO:
    with open(location + ".c", "w") as f:
        print(c_template.format(name=name, capital_name=capital_name), file=f)
    with open(location + ".h", "w") as f:
        print(header_template.format(name=name, capital_name=capital_name), file=f)
