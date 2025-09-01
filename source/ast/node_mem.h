
#pragma once
#include "node.h"
#include <stdint.h>
#include <stddef.h>


#define NODE_STACK_SIZE 1024 * 16


typedef struct {
    size_t  top;
    uint8_t stack[NODE_STACK_SIZE];
} node_stack;

typedef struct {
    uint8_t*   addr;
    const bool is_none;
} node_stack_ptr;


[[nodiscard]] node_stack_ptr get_top_of_g_node_stack();
[[nodiscard]] node_stack_ptr place_on_node_stack(uint8_t* node_addr);

void push_g_node_stack(const node_ptr ptr);


typedef struct node_stack_iter {
    size_t stack_pos;
    const uint8_t* (*next)(struct node_stack_iter* self);
} node_stack_iter;

[[nodiscard]] const uint8_t* node_stack_iter_next(node_stack_iter* self);

void node_stack_iter_constructor(node_stack_iter* self);


