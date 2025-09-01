
#pragma once
#include "asm_node.h"
#include <stdint.h>
#include <stddef.h>


#define ASM_NODE_STACK_SIZE 1024 * 16


typedef struct {
    size_t  top;
    uint8_t stack[ASM_NODE_STACK_SIZE];
} asm_node_stack;

typedef struct {
    uint8_t*   addr;
    const bool is_none;
} asm_node_stack_ptr;


[[nodiscard]] asm_node_stack_ptr get_top_of_g_asm_node_stack();
[[nodiscard]] asm_node_stack_ptr place_on_asm_node_stack(uint8_t* asm_node_addr);

void push_g_asm_node_stack(const asm_node_ptr ptr);


typedef struct asm_node_stack_iter {
    size_t stack_pos;
    const uint8_t* (*next)(struct asm_node_stack_iter* self);
} asm_node_stack_iter;

[[nodiscard]] const uint8_t* asm_node_stack_iter_next(asm_node_stack_iter* self);

void asm_node_stack_iter_constructor(asm_node_stack_iter* self);


