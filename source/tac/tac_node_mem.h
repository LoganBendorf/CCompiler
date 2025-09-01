
#pragma once
#include "tac_node.h"
#include <stdint.h>
#include <stddef.h>


#define TAC_NODE_STACK_SIZE 1024 * 16


typedef struct {
    size_t  top;
    uint8_t stack[TAC_NODE_STACK_SIZE];
} tac_node_stack;

typedef struct {
    uint8_t*   addr;
    const bool is_none;
} tac_node_stack_ptr;


[[nodiscard]] tac_node_stack_ptr get_top_of_g_tac_node_stack();
[[nodiscard]] tac_node_stack_ptr place_on_tac_node_stack(uint8_t* tac_node_addr);

void push_g_tac_node_stack(const tac_node_ptr ptr);


typedef struct tac_node_stack_iter {
    size_t stack_pos;
    const uint8_t* (*next)(struct tac_node_stack_iter* self);
} tac_node_stack_iter;

[[nodiscard]] const uint8_t* tac_node_stack_iter_next(tac_node_stack_iter* self);

void tac_node_stack_iter_constructor(tac_node_stack_iter* self);


