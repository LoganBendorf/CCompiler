
#include "tac_node.h"
#include "tac_node_mem.h"
#include "tac_node_size.h"

#include "structs.h"
#include "logger.h"

#include <stdint.h>
#include <string.h>



extern bool DEBUG;
extern bool TEST;

extern tac_node_stack g_tac_node_stack;



#define BUFFER_SIZE 1024 * 8


static size_t tac_node_addr_to_str(const uint8_t* addr, char* buf, const size_t buf_size, size_t index);


[[nodiscard]] string_span tac_node_span() {
    static const string arr[] = {
                {"PROGRAM_TAC_NODE", 16}, {"FUNCTION_TAC_NODE", 17}, {"RETURN_TAC_NODE", 15}, 
                {"BLOCK_TAC_NODE", 14}, {"INT_CONSTANT_TAC_NODE", 23}, {"UNARY_OP_TAC_NODE", 17}, {"BINARY_OP_TAC_NODE", 18} ,{"VAR_TAC_NODE", 14},
                {"IDENTIFIER_TAC_NODE", 19}
            };
    return (string_span){arr, sizeof(arr) / sizeof(arr[0]) };
}

string get_tac_node_string(const size_t index) {

    const string_span tac_node_strs = tac_node_span();

    if (index >= tac_node_strs.size) {
        FATAL_ERROR_STACK_TRACE("get_tac_node_string(): Out of bounds index, (%zu)", index); }

    return tac_node_strs.strs[index];
}

bool is_tac_expr(const tac_node_type type) {
    switch (type) {
    case INT_CONSTANT_TAC_NODE: case UNARY_OP_TAC_NODE: case VAR_TAC_NODE:
        return true;
    default: 
        return false;
    }
}
bool is_tac_stmt(const tac_node_type type) {
    switch (type) {
    case RETURN_TAC_NODE:
        return true;
    default: 
        return false;
    }
}



static size_t function_tac_node_to_str(const function_tac_node func_tac_nd, char* buf, const size_t buf_size, size_t index) {

    const string name = func_tac_nd.name;

    index += (size_t)snprintf(buf + index, buf_size - index, "  .globl %.*s\n%.*s:\n", (int) name.size, name.data, (int) name.size, name.data);

    const block_tac_node body = func_tac_nd.nodes;
    for (size_t i = 0; i < body.nodes.size; i++) {
        const tac_node_ptr tac_nd = body.nodes.values[i];
        // index += (size_t)snprintf(buf + index, buf_size - index, "  "); Should be done by stmts themselves
        index = tac_node_addr_to_str(tac_nd.addr, buf, buf_size, index);
        index += (size_t)snprintf(buf + index, buf_size - index, "\n");
    }

    // size += sprintf(buffer + size, "\n");


    return index;
}

static size_t unary_op_tac_node_to_str(const unary_op_tac_node un_op_tac_nd, char* buf, const size_t buf_size, size_t index) {

    const unary_op_type op_type  = un_op_tac_nd.op_type;
    const bool          has_prev = un_op_tac_nd.has_prev;
    const tac_expr_ptr  prev     = un_op_tac_nd.prev;
    const tac_expr_ptr  src      = un_op_tac_nd.src;
    const var_tac_node  dst      = un_op_tac_nd.dst;

    if (has_prev) {
        index = tac_node_addr_to_str( prev.addr, buf, buf_size, index);
    }

    const string dst_str = dst.name;
    index += (size_t)snprintf(buf + index, buf_size - index,"  %.*s = ", (int) dst_str.size, dst_str.data);

    // Pre
    bool printed_op = false;
    switch (op_type) {
    case NEGATE_OP:             index += (size_t)snprintf(buf + index, buf_size - index, "-");  printed_op = true; break;
    case BITWISE_COMPLEMENT_OP: index += (size_t)snprintf(buf + index, buf_size - index, "~");  printed_op = true; break;
    case PRE_DECREMENT_OP:      index += (size_t)snprintf(buf + index, buf_size - index, "--"); printed_op = true; break;
    default: break;
    }

    index = tac_node_addr_to_str(src.addr, buf, buf_size, index);
    
    // Post
    switch (op_type) {
    case POST_DECREMENT_OP: index += (size_t)snprintf(buf + index, buf_size - index, "--"); printed_op = true; break;
    default: break;
    }

    if (!printed_op) {
        FATAL_ERROR("Cannot print operator of type (%u), yes there's no string for it cope", op_type); }

    index += (size_t)snprintf(buf + index, buf_size - index,"\n");

    return index;
}

static size_t binary_op_tac_node_to_str(const binary_op_tac_node bin_op_tac_nd, char* buf, const size_t buf_size, size_t index) {

    const binary_op_type op_type = bin_op_tac_nd.op_type;
    const optional_tac_expr_ptr left_prev  = bin_op_tac_nd.left_prev;
    const optional_tac_expr_ptr right_prev = bin_op_tac_nd.right_prev;
    const tac_expr_ptr   left  = bin_op_tac_nd.left;
    const tac_expr_ptr   right = bin_op_tac_nd.right;
    const var_tac_node   dst   = bin_op_tac_nd.dst;

    
    if (left_prev.exists) {
        index = tac_node_addr_to_str( left.addr, buf, buf_size, index); }
    
    if (right_prev.exists) {
        index = tac_node_addr_to_str( right.addr, buf, buf_size, index); }
    

    const string dst_str = dst.name;
    index += (size_t)snprintf(buf + index, buf_size - index,"  %.*s = (", (int) dst_str.size, dst_str.data);


    if (left_prev.exists) {
        index = tac_node_addr_to_str(left_prev.value.addr, buf, buf_size, index);
    } else {
        index = tac_node_addr_to_str( left.addr, buf, buf_size, index);
    }


    switch (op_type) {
    case ADD_OP: index += (size_t)snprintf(buf + index, buf_size - index, "+"); break;
    case SUB_OP: index += (size_t)snprintf(buf + index, buf_size - index, "-"); break;
    case MUL_OP: index += (size_t)snprintf(buf + index, buf_size - index, "*"); break;
    case DIV_OP: index += (size_t)snprintf(buf + index, buf_size - index, "/"); break;
    case MOD_OP: index += (size_t)snprintf(buf + index, buf_size - index, "%%"); break;
    default: FATAL_ERROR("Cannot print binary operator of type (%.*s)", (int) get_binary_op_string(op_type).size, get_binary_op_string(op_type).data);
    }

    if (right_prev.exists) {
        index = tac_node_addr_to_str(right_prev.value.addr, buf, buf_size, index);
    } else {
        index = tac_node_addr_to_str( right.addr, buf, buf_size, index);
    }

    index += (size_t)snprintf(buf + index, buf_size - index,")\n");

    return index;
}

static size_t tac_node_addr_to_str(const uint8_t* addr, char* buf, const size_t buf_size, size_t index) {

    const tac_node_type type = *addr;

    const string tac_node_type_str = get_tac_node_string(type);
    
    switch (type) {

    case FUNCTION_TAC_NODE: {
        const function_tac_node func_tac_nd = *((function_tac_node*) addr);
        index = function_tac_node_to_str(func_tac_nd, buf, buf_size, index);
    } break;
    case PROGRAM_TAC_NODE: {
        const program_tac_node  prog_tac_nd = *((program_tac_node*) addr);
        const function_tac_node func_tac_nd = prog_tac_nd.main_function;
        index = function_tac_node_to_str(func_tac_nd, buf, buf_size, index);

        index += (size_t)snprintf(buf + index, buf_size - index,".section .note.GNU-stack,\"\",@progbits\n");
    } break;    
    case INT_CONSTANT_TAC_NODE: {
        const int_constant_tac_node int_const_tac_nd = *((int_constant_tac_node*) addr);
        index += (size_t)snprintf(buf + index, buf_size - index,"%d", int_const_tac_nd.value);
    } break;
    case UNARY_OP_TAC_NODE: {
        const unary_op_tac_node un_op_tac_nd = *((unary_op_tac_node*) addr);
        index = unary_op_tac_node_to_str(un_op_tac_nd, buf, buf_size, index);
    } break;
    case BINARY_OP_TAC_NODE: {
        const binary_op_tac_node bin_op_tac_nd = *((binary_op_tac_node*) addr);
        index = binary_op_tac_node_to_str(bin_op_tac_nd, buf, buf_size, index);
    } break;
    case VAR_TAC_NODE: {
        const var_tac_node var_tac_nd = *((var_tac_node*) addr);
        index += (size_t)snprintf(buf + index, buf_size - index,"%.*s", (int) var_tac_nd.name.size, var_tac_nd.name.data);
    } break;
    case RETURN_TAC_NODE: {
        const return_tac_node ret_tac_nd = *((return_tac_node*) addr);
        if (ret_tac_nd.expr_contains_dst) {
            index = tac_node_addr_to_str(ret_tac_nd.expr.addr, buf, buf_size, index);
            index += (size_t)snprintf(buf + index, buf_size - index,"  ret");
            index += (size_t)snprintf(buf + index, buf_size - index," %.*s", (int) ret_tac_nd.src.name.size, ret_tac_nd.src.name.data);
        } else {
            index += (size_t)snprintf(buf + index, buf_size - index,"  ret ");
            index = tac_node_addr_to_str(ret_tac_nd.expr.addr, buf, buf_size, index);
        }
    } break;


    default: 
        FATAL_ERROR("Cannot convert (%.*s) tac tac_node to string", (int) tac_node_type_str.size, tac_node_type_str.data);
        return 0;
    }


    return index;
}

void print_tac_program(const program_tac_node prog) {
    printf("PRINTING TAC PROGRAM -------------\n");
    static char buf[BUFFER_SIZE];
    const size_t start_index = 0;
    const size_t end_index = tac_node_addr_to_str((const uint8_t*) &prog, buf, BUFFER_SIZE, start_index);
    printf("%.*s", (int) end_index + 1, buf + start_index);
    printf("DONE -----------------------------\n\n");
}


void print_g_tac_nodes(void) {

    printf("PRINTING TAC NODES ---------------\n");

    size_t max_type_width = 0;
    size_t stack_pos = 0;
    size_t num_tac_nodes = 0;
    while (stack_pos < g_tac_node_stack.top) {

        
        const uint8_t* addr = g_tac_node_stack.stack + stack_pos;
        
        const tac_node_type type = *addr;
        
        const string tac_node_type_str = get_tac_node_string(type);
        max_type_width = tac_node_type_str.size > max_type_width ? tac_node_type_str.size : max_type_width;
        
        num_tac_nodes++;

        const size_t jump_size = size_of_tac_node((tac_node_type) *(addr));

        stack_pos += jump_size;
    }

    char str[32];
    sprintf(str, "%zu", num_tac_nodes);
    const size_t max_num_width = strlen(str) + 1; 



    stack_pos = 0;
    size_t count = 1;
    while (stack_pos < g_tac_node_stack.top) {

        const uint8_t* addr = g_tac_node_stack.stack + stack_pos;
        
        const tac_node_type type = *addr;

        const string tac_node_type_str = get_tac_node_string(type);
        
        static char buf[BUFFER_SIZE];
        const size_t start_index = 0;
        const size_t end_index = tac_node_addr_to_str(addr, buf, BUFFER_SIZE, start_index);

        bool found_newline = false;
        for (size_t i = 0; i < end_index; i++) {
            if (buf[i] == '\n') { found_newline = true; }
        }

        const char* add_newline = found_newline ? "\n" : "";

        const int pad_size = found_newline ? 0 : (int) max_type_width - (int) tac_node_type_str.size;

        printf("%*zu: Type (%.*s)%s%*s'%.*s'\n", 
            (int) max_num_width,          count++, 
            (int) tac_node_type_str.size, tac_node_type_str.data, 
                  add_newline,
                  pad_size                , "", 
            (int) end_index + 1,          buf + start_index
        );

        const size_t jump_size = size_of_tac_node((tac_node_type) *(addr));

        stack_pos += jump_size;
    }
    printf("DONE -----------------------------\n\n");
}


// var_tac_node
var_tac_node make_var_tac_node(const string set_name) {
    return (var_tac_node){VAR_TAC_NODE, set_name};
}

// int_constant_tac_node
int_constant_tac_node make_int_constant_tac_node(const int set_value){
    return (int_constant_tac_node){INT_CONSTANT_TAC_NODE, set_value};
}

// unary_op_tac_node
unary_op_tac_node make_unary_op_tac_node(const unary_op_type op_type, const tac_expr_ptr set_src, const var_tac_node set_dst) {
    return (unary_op_tac_node){UNARY_OP_TAC_NODE, .op_type=op_type, .has_prev=false, .src=set_src, .dst=set_dst};
}
unary_op_tac_node make_unary_op_tac_node_wth_previous(const unary_op_type op_type, const tac_expr_ptr prev, const tac_expr_ptr set_src, const var_tac_node set_dst) {
    return (unary_op_tac_node){UNARY_OP_TAC_NODE, .op_type=op_type, .has_prev=true, .prev=prev, .src=set_src, .dst=set_dst};
}

// binary_op_tac_node
binary_op_tac_node make_binary_op_tac_node(const binary_op_type op_type, const tac_expr_ptr set_left, const tac_expr_ptr set_right, const var_tac_node set_dst) {
    return (binary_op_tac_node){BINARY_OP_TAC_NODE, .op_type=op_type, .left_prev={.exists=false}, .right_prev={.exists=false}, .left=set_left, .right=set_right, .dst=set_dst};
}

binary_op_tac_node make_binary_op_tac_node_has_left_prev(const binary_op_type op_type, const tac_expr_ptr set_left, 
                                                                       const tac_expr_ptr set_left_prev, const tac_expr_ptr set_right, const var_tac_node set_dst) 
{
    return (binary_op_tac_node){BINARY_OP_TAC_NODE, .op_type=op_type, .left_prev={.exists=true, .value=set_left_prev}, .right_prev={.exists=false}, 
                                .left=set_left, .right=set_right, .dst=set_dst};
}
binary_op_tac_node make_binary_op_tac_node_has_right_prev(const binary_op_type op_type, const tac_expr_ptr set_left, const tac_expr_ptr set_right, 
                                                                        const tac_expr_ptr set_right_prev, const var_tac_node set_dst)
{
    return (binary_op_tac_node){BINARY_OP_TAC_NODE, .op_type=op_type, .left_prev={.exists=false}, .right_prev={.exists=true, .value=set_right_prev}, 
                                .left=set_left, .right=set_right, .dst=set_dst};
}
binary_op_tac_node make_binary_op_tac_node_has_both_prev(const binary_op_type op_type, const tac_expr_ptr set_left, const tac_expr_ptr set_right, 
                                                                        const tac_expr_ptr set_left_prev, const tac_expr_ptr set_right_prev, const var_tac_node set_dst)
{
    return (binary_op_tac_node){BINARY_OP_TAC_NODE, .op_type=op_type, .left_prev={.exists=true, .value=set_left_prev}, .right_prev={.exists=true, .value=set_right_prev}, 
                                .left=set_left, .right=set_right, .dst=set_dst};
}


// block_tac_node
block_tac_node make_block_tac_node(const tac_node_ptr_array set_statements) {
    return (block_tac_node){BLOCK_TAC_NODE, set_statements};
}

// function_tac_node
function_tac_node make_function_tac_node(const string set_name, const block_tac_node set_nodes) {
    return (function_tac_node){FUNCTION_TAC_NODE, set_name, set_nodes};
}

// program_tac_node
program_tac_node make_program_tac_node(const function_tac_node set_func) {
    return (program_tac_node){PROGRAM_TAC_NODE, set_func};
}

// return_tac_node
return_tac_node make_return_tac_node(const tac_expr_ptr set_expr, const bool expr_contains_dst, const var_tac_node set_src) {
    return (return_tac_node){RETURN_TAC_NODE, set_expr, expr_contains_dst, set_src};
}

// identifier_tac_node
identifier_tac_node make_identifier_tac_node(const string set_name) {
    return (identifier_tac_node){IDENTIFIER_TAC_NODE, set_name};

}
