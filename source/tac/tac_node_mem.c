
#include "tac_node_mem.h"
#include "tac_node_size.h"
#include "logger.h"
#include <string.h>
#include <stdio.h>


string get_tac_node_string(const size_t index);


extern bool DEBUG;
extern bool TEST;


static size_t prev_stack_push_size = 0;


tac_node_stack g_tac_node_stack = {0, {0}};


[[nodiscard]] static size_t size_of_tac_node_at_ptr(const tac_node_ptr ptr) { return size_of_tac_node((tac_node_type) *ptr.addr); }

[[nodiscard]] static tac_node_stack_ptr g_tac_node_stack_top_ptr() { return (tac_node_stack_ptr){&g_tac_node_stack.stack[g_tac_node_stack.top], false}; }


tac_node_stack_ptr get_top_of_g_tac_node_stack() {
    if (g_tac_node_stack.top == 0) { return (tac_node_stack_ptr){.addr=0, .is_none=true}; }

    const size_t size = prev_stack_push_size;
    if (size > g_tac_node_stack.top) {
        const uint8_t* top_addr = g_tac_node_stack_top_ptr().addr;
        const tac_node_type top_type = *top_addr;
        
        FATAL_ERROR_STACK_TRACE("Can not get top if global tac_node stack, would lead to underflow.\nTop = %zu, request size = %zu. Top type = (%.*s)", 
                                    g_tac_node_stack.top, size,
                                    (int) get_tac_node_string(top_type).size, get_tac_node_string(top_type).data); 
    }

    tac_node_stack_ptr ret = g_tac_node_stack_top_ptr();
    ret.addr -= size;

    return ret;
}

void push_g_tac_node_stack(const tac_node_ptr ptr) {
    if (ptr.addr == NULL) { FATAL_ERROR_STACK_TRACE("Can't push NULL pointer"); }

    const size_t size = size_of_tac_node_at_ptr(ptr);
    if (size + g_tac_node_stack.top >= TAC_NODE_STACK_SIZE) {
        FATAL_ERROR_STACK_TRACE("Stack OOM"); }

    memcpy((void*) g_tac_node_stack_top_ptr().addr,  (void*) ptr.addr, size);
    
    g_tac_node_stack.top += size;

    prev_stack_push_size = size;
}



const uint8_t* tac_node_stack_iter_next(tac_node_stack_iter* self) {
    if (self->stack_pos >= TAC_NODE_STACK_SIZE) { return NULL; }

    const uint8_t* addr = g_tac_node_stack.stack + self->stack_pos;
    const size_t jump_size = size_of_tac_node((tac_node_type) *(addr));

    self->stack_pos += jump_size;

    return addr;
}

void tac_node_stack_iter_constructor(tac_node_stack_iter* self) {
    self->stack_pos = 0;
    self->next = &tac_node_stack_iter_next;
}


