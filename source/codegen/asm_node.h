#pragma once

#include "registers.h"
#include "structs.h"
#include "unary_op.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>



#define ASM_NODE_STACK_SIZE 1024 * 16



void print_g_asm_nodes(const string msg);
[[nodiscard]] string get_asm_node_string(const size_t index);

void   set_top_traversal_index(const size_t val);
size_t get_g_asm_node_stack_top_value();
int    get_elem_size_index();
void   set_elem_size_traversal_index(const int val);

typedef enum { PROGRAM_ASM_NODE, FUNCTION_ASM_NODE, INT_IMMEDIATE_ASM_NODE,
               MOVE_ASM_NODE, RETURN_ASM_NODE, BLOCK_ASM_NODE, UNARY_OP_ASM_NODE, REGISTER_ASM_NODE, STACK_LOCATION_ASM_NODE, IDENTIFIER_ASM_NODE,
               TEMP_REG_ASM_NODE
} asm_node_type;

[[nodiscard]] bool is_operandanble(const asm_node_type type);


typedef struct {
    const size_t allignment;
    size_t       top;
    uint8_t      stack[ASM_NODE_STACK_SIZE];
} asm_node_stack;

typedef struct {
    uint8_t* addr;
    const bool     is_none;
} asm_node_stack_ptr;

[[nodiscard]] asm_node_stack_ptr get_top_of_g_asm_node_stack();
[[nodiscard]] asm_node_stack_ptr pop_g_asm_node_stack();

typedef struct {
    const uint8_t* addr;
} asm_node_ptr;

void push_g_asm_node_stack(const asm_node_ptr ptr);

typedef struct {
    const uint8_t* addr;
} asm_expr_ptr;


typedef struct {
    const asm_node_ptr* values;
    const size_t     size;
} asm_node_ptr_array;

typedef struct {
     const uint8_t* addr;
} asm_operand_ptr;





typedef struct {
    const asm_node_type      type;
    const asm_node_ptr_array nodes;
} block_asm_node;

[[nodiscard]] block_asm_node make_block_asm_node(const asm_node_ptr_array set_nodes);

typedef struct {
    const asm_node_type  type;
    const string         name;
    const block_asm_node nodes;
    int stack_allocate_ammount;
} function_asm_node;

[[nodiscard]] function_asm_node make_function_asm_node(const string set_name, const block_asm_node set_nodes);



typedef struct {
    const asm_node_type     type;
    const function_asm_node main_function;
} program_asm_node;

[[nodiscard]] program_asm_node make_program_asm_node(const function_asm_node set_func);
void                           print_asm_program(const program_asm_node prog, const string msg);
[[nodiscard]] string           get_assembly(const program_asm_node prog);


typedef struct {
    const asm_node_type type;
    const int           value;
} int_immediate_asm_node;

[[nodiscard]] int_immediate_asm_node make_int_immediate_asm_node(const int set_value);



typedef struct {
    const asm_node_type type;
    const string        name;
} temp_reg_asm_node;

[[nodiscard]] temp_reg_asm_node make_temp_reg_asm_node(const string set_name);

typedef struct { // Only used in second ASM pass to remove temp vars
    const asm_node_type type;
    const int           position;
} stack_loc_asm_node;

[[nodiscard]] stack_loc_asm_node make_stack_loc_asm_node(const int set_position);


typedef struct {
    const asm_node_type type;
    bool is_temp;
    bool is_stack;
    union {
        const temp_reg_asm_node temp_reg;
        stack_loc_asm_node      stack_loc;
        const x86_register      reg;
    };
} register_asm_node;

[[nodiscard]] register_asm_node* make_register_asm_node(const temp_reg_asm_node set_temp_reg);
[[nodiscard]] register_asm_node* make_register_asm_node_with_stack_loc(const stack_loc_asm_node stack_loc);
[[nodiscard]] register_asm_node  make_register_asm_node_with_stack_loc_dont_place_on_stack(const stack_loc_asm_node stack_loc);
[[nodiscard]] register_asm_node* make_register_asm_node_with_x86(const x86_register set_reg);


typedef struct {
    const asm_node_type      type;
    const unary_op_type      op_type;
    const bool               has_prev;
    const asm_expr_ptr       prev;
    const asm_expr_ptr       src; // Can be constants or regs
    const register_asm_node* dst_reg;
} unary_op_asm_node;

[[nodiscard]] unary_op_asm_node make_unary_op_asm_node(const unary_op_type op_type, const asm_expr_ptr set_src, const register_asm_node* set_dst);
[[nodiscard]] unary_op_asm_node make_unary_op_asm_node_wth_previous(const unary_op_type op_type, const asm_expr_ptr prev, const asm_expr_ptr set_src, const register_asm_node* set_dst);


typedef struct {
    const asm_node_type     type;
    const register_asm_node* src;
    const register_asm_node* dst;
} move_asm_node;

[[nodiscard]] move_asm_node make_move_asm_node(const register_asm_node* set_src, const register_asm_node* set_dst);


typedef struct {
    const asm_node_type type;
    const asm_expr_ptr  expr;
    const move_asm_node move_nd;
} return_asm_node;

[[nodiscard]] return_asm_node make_return_asm_node(const asm_expr_ptr set_expr, const move_asm_node set_move_nd);

typedef struct { // '-' for push & '+' for pop
    const asm_node_type type;
    const int velocity;
} stack_mover_asm_node;

[[nodiscard]] stack_mover_asm_node make_stack_mover_asm_node(const int set_velocity);


typedef struct {
    const asm_node_type type;
    const string        name;
} identifier_asm_node;

[[nodiscard]] identifier_asm_node make_identifier_asm_node(const string set_name);
