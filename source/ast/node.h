#pragma once

#include "structs.h"
#include "token.h"
#include "unary_op.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>



#define NODE_STACK_SIZE 1024 * 16



typedef enum { NULL_NODE, PROGRAM_NODE, FUNCTION_NODE, IDENTIFIER_NODE, BLOCK_STATEMENT_NODE, STATEMENT_NODE, RETURN_STATEMENT_NODE, INTEGER_CONSTANT_NODE, IF_STATEMENT_NODE,
               CUSTOM_TYPE_NODE, C_TYPE_NODE, TYPE_NODE, UNARY_OP_NODE, BINARY_OP_NODE
} node_type;

typedef enum {
    NONE_EXIST_YET_RN_FR_FR,
    ADD_OP, SUB_OP, MUL_OP, DIV_OP,
    EQUALS_OP, NOT_EQUALS_OP, LESS_THAN_OP, LESS_THAN_OR_EQUAL_TO_OP, GREATER_THAN_OP, GREATER_THAN_OR_EQUAL_TO_OP,
    DOT_OP
} binary_op_type;



[[nodiscard]] bool is_expression(const node_type type);
[[nodiscard]] bool is_statement (const node_type type);
              void print_g_nodes(void);
[[nodiscard]] string get_node_string(const size_t index);

typedef struct {
    const size_t allignment;
    size_t       top;
    uint8_t      stack[NODE_STACK_SIZE];
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
[[nodiscard]] node_stack_ptr get_top_of_g_node_stack();

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

[[nodiscard]] integer_constant_node make_integer_constant_node(const int value);
[[nodiscard]] integer_constant_node make_integer_constant_node_with_str(const string value);


typedef struct {
    const node_type     type;
    const unary_op_type op_type;
    const expr_ptr      expr;
} unary_op_node;

[[nodiscard]] unary_op_node make_unary_op_node(const unary_op_type set_op_type, const expr_ptr set_expr);


typedef struct {
    const node_type type;
    const expr_ptr  expr;
} return_statement_node;

[[nodiscard]] return_statement_node make_return_statement_node(const expr_ptr expr);


typedef struct {
    const node_type STATEMENT_NODE;
    expr_ptr        expr;
} statement_node;


typedef struct {
    const node_type      type;
    const stmt_ptr_array statements; // TODO Need vector? Malloc?
} block_statement_node;

[[nodiscard]] block_statement_node make_block_statement_node(const stmt_ptr_array set_statements);


typedef struct {
    const node_type type;
    const string    value;
} identifier_node;

[[nodiscard]] identifier_node make_identifier_node(const string set_value);


typedef struct {
    const node_type       CUSTOM_TYPE_NODE;
    const identifier_node ident;
} custom_type_node; // Used waaaay later for typedefs


typedef struct {
    const node_type  type;
    const token_type c_token_type;
} c_type_node;

[[nodiscard]] c_type_node make_c_type_node(const token_type set_type);


typedef struct {
    const node_type TYPE_NODE;
    bool is_c_type;
    union {
        c_type_node      c_type;
        custom_type_node custom_type;
    } type;
} type_node;

[[nodiscard]] type_node make_type_node_c_type_token(const token_type set_type);
[[nodiscard]] type_node make_type_node_c_type_node (const c_type_node set_type);
// type_node make_type_node_custom_type(const custom_type_node set_type);


typedef struct {
    const node_type            type;
    const type_node            return_type;
    const string               name;
    const type_node            parameter; // TODO Only one param for now (void)
    const block_statement_node body;
} function_node;

[[nodiscard]] function_node make_function_node(const type_node set_return_type, const string set_name, const type_node set_parameter, const block_statement_node set_body);


typedef struct {
    const node_type     type;
    const function_node main_function;
} program_node;

[[nodiscard]] program_node make_program_node(const function_node set_func);
void print_program_ast(const program_node prog);


typedef struct {
    const node_type IF_STATEMENT_NODE;
    const expr_ptr  condition;
    const stmt_ptr  then_block;
    bool            has_else;
    stmt_ptr        else_block;
} if_statement_node;




