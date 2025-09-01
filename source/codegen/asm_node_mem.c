
#include "asm_node_mem.h"
#include "asm_node_size.h"
#include "logger.h"
#include <string.h>
#include <stdio.h>


string get_asm_node_string(const size_t index);


extern bool DEBUG;
extern bool TEST;


static size_t prev_stack_push_size = 0;


asm_node_stack g_asm_node_stack = {0, {0}};


[[nodiscard]] static size_t size_of_asm_node_at_ptr(const asm_node_ptr ptr) { return size_of_asm_node((asm_node_type) *ptr.addr); }

[[nodiscard]] static asm_node_stack_ptr g_asm_node_stack_top_ptr() { return (asm_node_stack_ptr){&g_asm_node_stack.stack[g_asm_node_stack.top], false}; }


asm_node_stack_ptr get_top_of_g_asm_node_stack() {
    if (g_asm_node_stack.top == 0) { return (asm_node_stack_ptr){.addr=0, .is_none=true}; }

    const size_t size = prev_stack_push_size;
    if (size > g_asm_node_stack.top) {
        const uint8_t* top_addr = g_asm_node_stack_top_ptr().addr;
        const asm_node_type top_type = *top_addr;
        
        FATAL_ERROR_STACK_TRACE("Can not get top if global asm_node stack, would lead to underflow.\nTop = %zu, request size = %zu. Top type = (%.*s)", 
                                    g_asm_node_stack.top, size,
                                    (int) get_asm_node_string(top_type).size, get_asm_node_string(top_type).data); 
    }

    asm_node_stack_ptr ret = g_asm_node_stack_top_ptr();
    ret.addr -= size;

    return ret;
}

void push_g_asm_node_stack(const asm_node_ptr ptr) {
    if (ptr.addr == NULL) { FATAL_ERROR_STACK_TRACE("Can't push NULL pointer"); }

    const size_t size = size_of_asm_node_at_ptr(ptr);
    if (size + g_asm_node_stack.top >= ASM_NODE_STACK_SIZE) {
        FATAL_ERROR_STACK_TRACE("Stack OOM"); }

    memcpy((void*) g_asm_node_stack_top_ptr().addr,  (void*) ptr.addr, size);
    
    g_asm_node_stack.top += size;

    prev_stack_push_size = size;
}



const uint8_t* asm_node_stack_iter_next(asm_node_stack_iter* self) {
    if (self->stack_pos >= ASM_NODE_STACK_SIZE) { return NULL; }

    const uint8_t* addr = g_asm_node_stack.stack + self->stack_pos;
    const size_t jump_size = size_of_asm_node((asm_node_type) *(addr));

    self->stack_pos += jump_size;

    return addr;
}

void asm_node_stack_iter_constructor(asm_node_stack_iter* self) {
    self->stack_pos = 0;
    self->next = &asm_node_stack_iter_next;
}


