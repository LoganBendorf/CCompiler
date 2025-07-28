#pragma once

#include "structs.h"
#include "token.h"
#include "helpers.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>



#define node_STACK_SIZE 1024 * 16



typedef enum { PROGRAM_NODE, FUNCTION_NODE, IDENTIFIER_NODE, BLOCK_STATEMENT_NODE, STATEMENT_NODE, RETURN_STATEMENT_NODE, INTEGER_CONSTANT_NODE, IF_STATEMENT_NODE,
               CUSTOM_TYPE_NODE, C_TYPE_NODE, TYPE_NODE
} node_type;

bool is_expression(const node_type type);
bool is_statement (const node_type type);
void print_g_nodes(void);

typedef struct {
    const size_t allignment;
    size_t       top;
    uint8_t      stack[node_STACK_SIZE];
} node_stack;

typedef struct {
    const uint8_t* addr;
    const bool     is_none;
} node_stack_ptr;

typedef struct {
    const uint8_t* addr;
    const bool     is_none;
} node_ptr;


void push_g_node_stack(const node_ptr ptr);
node_stack_ptr get_top_of_g_node_stack();

typedef struct {
    const node_ptr* values;
    const size_t    size;
} node_ptr_array;

typedef struct {
    const uint8_t* addr;
} expr_ptr;

typedef struct {
    const uint8_t* addr;
} stmt_ptr;

typedef struct {
    const stmt_ptr* values;
    const size_t    size;
} stmt_ptr_array;


// Nodes

typedef struct {
    const node_type type;
    int             value;
} integer_constant_node;

integer_constant_node make_integer_constant_node(const int value);
integer_constant_node make_integer_constant_node_with_str(const string value);



typedef struct {
    const node_type type;
    const expr_ptr  expr;
} return_statement_node;

return_statement_node make_return_statement_node(const expr_ptr expr);



typedef struct {
    const node_type STATEMENT_NODE;
    expr_ptr        expr;
} statement_node;

typedef struct {
    const node_type BLOCK_STATEMENT_NODE;
    stmt_ptr_array  statements; // TODO Need vector? Malloc?
} block_statement_node;

typedef struct {
    const node_type IDENTIFIER_NODE;
    const token     value;
} identifier_node;

typedef struct {
    const node_type       CUSTOM_TYPE_NODE;
    const identifier_node ident;
} custom_type_node; // Used waaaay later for typedefs

typedef struct {
    const node_type  C_TYPE_NODE;
    const token_type type;
} c_type_node;

typedef struct {
    const node_type TYPE_NODE;
    union {
        c_type_node      c_type;
        custom_type_node custom_type;
    } type;
} type_node;

typedef struct {
    const node_type            FUNCTION_NODE;
    const type_node            type;
    const identifier_node      ident;
    const block_statement_node body;
} function_node;

typedef struct {
    const node_type     PROGRAM_NODE;
    const function_node main_function;
} program_node;

typedef struct {
    const node_type IF_STATEMENT_NODE;
    const expr_ptr  condition;
    const stmt_ptr  then_block;
    bool            has_else;
    stmt_ptr        else_block;
} if_statement_node;




