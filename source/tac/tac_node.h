#pragma once

#include "binary_op.h"
#include "structs.h"
#include "unary_op.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>



#define TAC_NODE_STACK_SIZE 1024 * 16


void print_g_tac_nodes(void);
[[nodiscard]] string get_tac_node_string(const size_t index);


typedef enum { PROGRAM_TAC_NODE, FUNCTION_TAC_NODE,
                RETURN_TAC_NODE, BLOCK_TAC_NODE, INT_CONSTANT_TAC_NODE, UNARY_OP_TAC_NODE, BINARY_OP_TAC_NODE, VAR_TAC_NODE, IDENTIFIER_TAC_NODE
} tac_node_type;

bool is_tac_expr(const tac_node_type type);
bool is_tac_stmt(const tac_node_type type);


typedef struct {
    const uint8_t* addr;
} tac_node_ptr;


typedef struct {
    const uint8_t* addr;
} tac_expr_ptr;

typedef struct {
    const uint8_t* addr;
} tac_stmt_ptr;


typedef struct {
    const tac_node_ptr* values;
    const size_t        size;
} tac_node_ptr_array;



typedef struct {
    const tac_node_type type;
    const string        name;
} var_tac_node;

[[nodiscard]] var_tac_node make_var_tac_node(const string set_name);

typedef struct {
    const bool is_error;
    union {
        const var_tac_node var;
        const string       error;
    };
} expect_var_tac_node;
// [[nodiscard]] expect_var_tac_node get_tac_dst(const uint8_t* addr); // FIXME Not needed?


typedef struct {
    const tac_node_type type;
    int                   value;
} int_constant_tac_node;

[[nodiscard]] int_constant_tac_node make_int_constant_tac_node(const int set_value);

typedef struct { // Is a linked list of combined expressions (e.g ~-12) will create 2 nodes
    const tac_node_type type;
    const unary_op_type op_type;
    const bool          has_prev;
    const tac_expr_ptr  prev;
    const tac_expr_ptr  src; // Can be constants or vars
    const var_tac_node  dst;
} unary_op_tac_node;

[[nodiscard]] unary_op_tac_node make_unary_op_tac_node(const unary_op_type op_type, const tac_expr_ptr set_src, const var_tac_node set_dst);
[[nodiscard]] unary_op_tac_node make_unary_op_tac_node_wth_previous(const unary_op_type op_type, const tac_expr_ptr prev, const tac_expr_ptr set_src, const var_tac_node set_dst);

typedef struct {
    const bool         exists;
    const tac_expr_ptr value;
} optional_tac_expr_ptr;


typedef struct { // Is a linked list of combined expressions (e.g 1 + (1 * 1)) will create 2 nodes, 1 stored in right_prev
    const tac_node_type         type;
    const binary_op_type        op_type;
    const optional_tac_expr_ptr left_prev;
    const optional_tac_expr_ptr right_prev;
    const tac_expr_ptr          left;
    const tac_expr_ptr          right;
    const var_tac_node          dst;
} binary_op_tac_node;

[[nodiscard]] binary_op_tac_node make_binary_op_tac_node(const binary_op_type op_type, const tac_expr_ptr set_left, const tac_expr_ptr set_right, const var_tac_node set_dst);
[[nodiscard]] binary_op_tac_node make_binary_op_tac_node_has_left_prev(const binary_op_type op_type, const tac_expr_ptr set_left, 
                                                                       const tac_expr_ptr set_left_prev, const tac_expr_ptr set_right, const var_tac_node set_dst);
[[nodiscard]] binary_op_tac_node make_binary_op_tac_node_has_right_prev(const binary_op_type op_type, const tac_expr_ptr set_left, const tac_expr_ptr set_right, 
                                                                        const tac_expr_ptr set_right_prev, const var_tac_node set_dst);
[[nodiscard]] binary_op_tac_node make_binary_op_tac_node_has_both_prev(const binary_op_type op_type, const tac_expr_ptr set_left, const tac_expr_ptr set_right, 
                                                                        const tac_expr_ptr set_left_prev, const tac_expr_ptr set_right_prev, const var_tac_node set_dst);




typedef struct {
    const tac_node_type      type;
    const tac_node_ptr_array nodes;
} block_tac_node;

[[nodiscard]] block_tac_node make_block_tac_node(const tac_node_ptr_array set_nodes);


typedef struct {
    const tac_node_type  type;
    const string         name;
    const block_tac_node nodes;
} function_tac_node;

[[nodiscard]] function_tac_node make_function_tac_node(const string set_name, const block_tac_node set_nodes);


typedef struct {
    const tac_node_type     type;
    const function_tac_node main_function;
} program_tac_node;

[[nodiscard]] program_tac_node make_program_tac_node(const function_tac_node set_func);
void print_tac_program(const program_tac_node prog);


typedef struct {
    const tac_node_type type;
    const tac_expr_ptr  expr;
    const bool expr_contains_dst;
    const var_tac_node  src;
} return_tac_node;

[[nodiscard]] return_tac_node make_return_tac_node(const tac_expr_ptr set_expr, const bool expr_contains_dst, const var_tac_node set_src);

typedef struct {
    const tac_node_type type;
    const string        name;
} identifier_tac_node;

[[nodiscard]] identifier_tac_node make_identifier_tac_node(const string set_name);

